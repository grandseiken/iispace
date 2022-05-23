#include "game/render/gl_renderer.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <lodepng.h>
#include <nonstd/span.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace ii {
namespace {
#include "game/render/shaders/legacy_line.f.glsl.h"
#include "game/render/shaders/legacy_line.v.glsl.h"
#include "game/render/shaders/legacy_rect.f.glsl.h"
#include "game/render/shaders/legacy_rect.v.glsl.h"
#include "game/render/shaders/legacy_text.f.glsl.h"
#include "game/render/shaders/legacy_text.v.glsl.h"

struct shader_source {
  shader_source(const char* name, gl::shader_type type, nonstd::span<const std::uint8_t> source)
  : filename{name}, type{type}, source{source} {
    std::replace(filename.begin(), filename.end(), '_', '.');
  }

  std::string filename;
  gl::shader_type type;
  nonstd::span<const std::uint8_t> source;
};

#define SHADER_SOURCE(name, type) \
  { #name, type, {game_render_shaders_##name, game_render_shaders_##name##_len }, }

const shader_source
    kLegacyLineFragmentShader SHADER_SOURCE(legacy_line_f_glsl, gl::shader_type::kFragment);
const shader_source
    kLegacyLineVertexShader SHADER_SOURCE(legacy_line_v_glsl, gl::shader_type::kVertex);
const shader_source
    kLegacyRectFragmentShader SHADER_SOURCE(legacy_rect_f_glsl, gl::shader_type::kFragment);
const shader_source
    kLegacyRectVertexShader SHADER_SOURCE(legacy_rect_v_glsl, gl::shader_type::kVertex);
const shader_source
    kLegacyTextFragmentShader SHADER_SOURCE(legacy_text_f_glsl, gl::shader_type::kFragment);
const shader_source
    kLegacyTextVertexShader SHADER_SOURCE(legacy_text_v_glsl, gl::shader_type::kVertex);

result<gl::shader> compile_shader(const shader_source& source) {
  auto shader = gl::compile_shader(source.type, source.source);
  if (!shader) {
    return unexpected("Couldn't compile " + source.filename + ": " + shader.error());
  }
  return {std::move(shader)};
}

template <typename T, std::size_t... I>
result<gl::program> link_program(const T& shaders, std::index_sequence<I...>) {
  return gl::link_program(*shaders[I]...);
}

template <typename... T>
result<gl::program> compile_program(const char* name, T&&... sources) {
  result<gl::shader> shaders[] = {compile_shader(sources)...};
  for (std::size_t i = 0; i < sizeof...(T); ++i) {
    if (!shaders[i]) {
      return unexpected(shaders[i].error());
    }
  }
  auto result = link_program(shaders, std::index_sequence_for<T...>{});
  if (!result) {
    return unexpected("Couldn't link " + std::string{name} + ": " + result.error());
  }
  return result;
}

result<gl::texture> load_png(const std::string& filename) {
  std::vector<std::uint8_t> image_bytes;  // 4bpp, RGBARGBA...
  unsigned width = 0, height = 0;
  unsigned error = lodepng::decode(image_bytes, width, height, filename);
  if (error) {
    return unexpected("Couldn't load " + filename + ": " + std::string{lodepng_error_text(error)});
  }
  auto texture = gl::make_texture();
  gl::texture_image_2d(texture, 0, gl::internal_format::kRgba8, {width, height},
                       gl::texture_format::kRgba, gl::type_of<std::byte>(),
                       nonstd::span<const std::uint8_t>{image_bytes});
  return {std::move(texture)};
}

}  // namespace

struct GlRenderer::impl_t {
  result<void> status;
  glm::uvec2 screen_dimensions{0, 0};
  glm::uvec2 render_dimensions{0, 0};

  gl::program legacy_line;
  gl::program legacy_rect;
  gl::program legacy_text;

  gl::buffer line_index;
  gl::buffer quad_index;
  gl::texture console_font;
  gl::sampler pixel_sampler;

  impl_t(gl::program legacy_line, gl::program legacy_rect, gl::program legacy_text,
         gl::texture console_font)
  : legacy_line{std::move(legacy_line)}
  , legacy_rect{std::move(legacy_rect)}
  , legacy_text{std::move(legacy_text)}
  , line_index(gl::make_buffer())
  , quad_index(gl::make_buffer())
  , console_font{std::move(console_font)}
  , pixel_sampler{gl::make_sampler(gl::filter::kNearest, gl::filter::kNearest,
                                   gl::texture_wrap::kClampToEdge,
                                   gl::texture_wrap::kClampToEdge)} {
    static const std::vector<unsigned> line_indices = {0, 1};
    static const std::vector<unsigned> quad_indices = {0, 1, 2, 0, 2, 3};
    gl::buffer_data(line_index, gl::buffer_usage::kStaticDraw, nonstd::span{line_indices});
    gl::buffer_data(quad_index, gl::buffer_usage::kStaticDraw, nonstd::span{quad_indices});
  }

  glm::vec2 legacy_render_scale() const {
    auto screen = static_cast<glm::vec2>(screen_dimensions);
    auto render = static_cast<glm::vec2>(render_dimensions);
    auto screen_aspect = screen.x / screen.y;
    auto render_aspect = render.x / render.y;

    if (screen_aspect > render_aspect) {
      return {render_aspect / screen_aspect, 1.f};
    } else {
      return {1.f, screen_aspect / render_aspect};
    }
  }

  void draw_line_internal(const glm::ivec2& a, const glm::ivec2& b) {
    using int_t = std::int16_t;
    const std::vector<int_t> vertex_data = {
        static_cast<int_t>(a.x),
        static_cast<int_t>(a.y),
        static_cast<int_t>(b.x),
        static_cast<int_t>(b.y),
    };
    auto vertex_buffer = gl::make_buffer();
    gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw, nonstd::span{vertex_data});

    auto vertex_array = gl::make_vertex_array();
    gl::bind_vertex_array(vertex_array);
    auto position_handle = gl::vertex_int_attribute_buffer(
        vertex_buffer, 0, 2, gl::type_of<int_t>(), 2 * sizeof(int_t), 0);
    gl::draw_elements(gl::draw_mode::kLines, line_index, gl::type_of<unsigned>(), 2, 0);
  }

  void draw_rect_internal(const glm::ivec2& position, const glm::ivec2& size) {
    using int_t = std::int16_t;
    const std::vector<int_t> vertex_data = {
        static_cast<int_t>(position.x),
        static_cast<int_t>(position.y),
        static_cast<int_t>(0),
        static_cast<int_t>(0),
        static_cast<int_t>(position.x),
        static_cast<int_t>(position.y + size.y),
        static_cast<int_t>(0),
        static_cast<int_t>(1),
        static_cast<int_t>(position.x + size.x),
        static_cast<int_t>(position.y + size.y),
        static_cast<int_t>(1),
        static_cast<int_t>(1),
        static_cast<int_t>(position.x + size.x),
        static_cast<int_t>(position.y),
        static_cast<int_t>(1),
        static_cast<int_t>(0),
    };
    auto vertex_buffer = gl::make_buffer();
    gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw, nonstd::span{vertex_data});

    auto vertex_array = gl::make_vertex_array();
    gl::bind_vertex_array(vertex_array);
    auto position_handle = gl::vertex_int_attribute_buffer(
        vertex_buffer, 0, 2, gl::type_of<int_t>(), 4 * sizeof(int_t), 0);
    auto tex_coords_handle = gl::vertex_int_attribute_buffer(
        vertex_buffer, 1, 2, gl::type_of<int_t>(), 4 * sizeof(int_t), 2 * sizeof(int_t));
    gl::draw_elements(gl::draw_mode::kTriangles, quad_index, gl::type_of<unsigned>(), 6, 0);
  }
};

result<GlRenderer> GlRenderer::create() {
  auto console_font = load_png("console.png");
  if (!console_font) {
    return unexpected(console_font.error());
  }

  auto legacy_line =
      compile_program("legacy_line.glsl", kLegacyLineFragmentShader, kLegacyLineVertexShader);
  if (!legacy_line) {
    return unexpected(legacy_line.error());
  }

  auto legacy_rect =
      compile_program("legacy_rect.glsl", kLegacyRectFragmentShader, kLegacyRectVertexShader);
  if (!legacy_rect) {
    return unexpected(legacy_rect.error());
  }

  auto legacy_text =
      compile_program("legacy_text.glsl", kLegacyTextFragmentShader, kLegacyTextVertexShader);
  if (!legacy_text) {
    return unexpected(legacy_text.error());
  }

  GlRenderer renderer{{}};
  renderer.impl_ = std::make_unique<impl_t>(std::move(*legacy_line), std::move(*legacy_rect),
                                            std::move(*legacy_text), std::move(*console_font));
  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
GlRenderer::GlRenderer(GlRenderer&&) = default;
GlRenderer& GlRenderer::operator=(GlRenderer&&) = default;
GlRenderer::~GlRenderer() = default;

void GlRenderer::clear_screen() {
  gl::clear_colour({0.f, 0.f, 0.f, 0.f});
  gl::clear(gl::clear_mask::kColourBufferBit | gl::clear_mask::kDepthBufferBit |
            gl::clear_mask::kStencilBufferBit);
}

void GlRenderer::set_dimensions(const glm::uvec2& screen_dimensions,
                                const glm::uvec2& render_dimensions) {
  impl_->screen_dimensions = screen_dimensions;
  impl_->render_dimensions = render_dimensions;
}

void GlRenderer::render_legacy_text(const glm::ivec2& position, const glm::vec4& colour,
                                    std::string_view text) {
  static constexpr glm::uvec2 kTextDimensions{16u, 16u};
  static constexpr std::size_t kCharacterCount = 128u;

  const auto& program = impl_->legacy_text;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto result = gl::set_uniforms(
      program, "render_scale", impl_->legacy_render_scale(), "render_dimensions",
      impl_->render_dimensions, "texture_dimensions",
      glm::uvec2{kCharacterCount * kTextDimensions.x, kTextDimensions.y}, "text_colour", colour);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }
  result = gl::set_uniform_texture_2d(program, "font_texture", 0, impl_->console_font,
                                      impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }

  std::vector<std::int32_t> vertex_data;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto c = text[i];
    if (c >= kCharacterCount) {
      continue;
    }
    auto text_i = static_cast<std::int32_t>(i);
    auto char_i = static_cast<std::int32_t>(c);
    auto text_x = static_cast<std::int32_t>(kTextDimensions.x);
    auto text_y = static_cast<std::int32_t>(kTextDimensions.y);

    vertex_data.emplace_back((text_i + position.x) * text_x);
    vertex_data.emplace_back(position.y * text_y);

    vertex_data.emplace_back(char_i * text_x);
    vertex_data.emplace_back(0);

    vertex_data.emplace_back((text_i + position.x) * text_x);
    vertex_data.emplace_back((1 + position.y) * text_y);

    vertex_data.emplace_back(char_i * text_x);
    vertex_data.emplace_back(text_y);

    vertex_data.emplace_back((1 + text_i + position.x) * text_x);
    vertex_data.emplace_back((1 + position.y) * text_y);

    vertex_data.emplace_back((1 + char_i) * text_x);
    vertex_data.emplace_back(text_y);

    vertex_data.emplace_back((1 + text_i + position.x) * text_x);
    vertex_data.emplace_back(position.y * text_y);

    vertex_data.emplace_back((1 + char_i) * text_x);
    vertex_data.emplace_back(0);
  }

  auto vertex_buffer = gl::make_buffer();
  gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw,
                  nonstd::span<const std::int32_t>{vertex_data});

  auto quads = static_cast<unsigned>(vertex_data.size() / 16);
  std::vector<unsigned> indices;
  for (unsigned i = 0; i < quads; ++i) {
    indices.insert(indices.end(), {4 * i, 4 * i + 1, 4 * i + 2, 4 * i, 4 * i + 2, 4 * i + 3});
  }

  auto index_buffer = gl::make_buffer();
  gl::buffer_data(index_buffer, gl::buffer_usage::kStreamDraw,
                  nonstd::span<const unsigned>{indices});

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);
  auto position_handle = gl::vertex_int_attribute_buffer(
      vertex_buffer, 0, 2, gl::type_of<std::int32_t>(), 4 * sizeof(std::int32_t), 0);
  auto tex_coords_handle =
      gl::vertex_int_attribute_buffer(vertex_buffer, 1, 2, gl::type_of<std::int32_t>(),
                                      4 * sizeof(std::int32_t), 2 * sizeof(std::int32_t));
  gl::draw_elements(gl::draw_mode::kTriangles, index_buffer, gl::type_of<unsigned>(), quads * 6, 0);
}

void GlRenderer::render_legacy_line(const glm::vec2& a, const glm::vec2& b,
                                    const glm::vec4& colour) {
  const auto& program = impl_->legacy_line;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto result =
      gl::set_uniforms(program, "render_scale", impl_->legacy_render_scale(), "render_dimensions",
                       impl_->render_dimensions, "line_colour", colour);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }
  impl_->draw_line_internal(a, b);
}

void GlRenderer::render_legacy_rect(const glm::ivec2& lo, const glm::ivec2& hi,
                                    std::int32_t line_width, const glm::vec4& colour) {
  const auto& program = impl_->legacy_rect;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto size = hi - lo;
  auto result =
      gl::set_uniforms(program, "render_scale", impl_->legacy_render_scale(), "render_dimensions",
                       impl_->render_dimensions, "rect_dimensions", static_cast<glm::uvec2>(size),
                       "line_width", glm::uvec2{line_width, line_width}, "rect_colour", colour);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }
  impl_->draw_rect_internal(lo, size);
}

}  // namespace ii