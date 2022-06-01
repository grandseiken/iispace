#include "game/render/gl_renderer.h"
#include "game/common/raw_ptr.h"
#include "game/io/file/filesystem.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <nonstd/span.hpp>
#include <stb_image.h>
#include <algorithm>
#include <string>
#include <vector>

namespace ii::render {
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

result<gl::texture> load_image(const std::string& filename, nonstd::span<const std::uint8_t> data) {
  int width = 0;
  int height = 0;
  int channels = 0;
  auto image_bytes = make_raw(
      stbi_load_from_memory(data.data(), data.size(), &width, &height, &channels,
                            /* desired channels */ 4),
      +[](std::uint8_t* p) { stbi_image_free(p); });
  if (!image_bytes) {
    return unexpected("Couldn't parse " + filename + ": " + std::string{stbi_failure_reason()});
  }
  glm::uvec2 dimensions{static_cast<unsigned>(width), static_cast<unsigned>(height)};
  auto texture = gl::make_texture();
  gl::texture_image_2d(
      texture, 0, gl::internal_format::kRgba8, dimensions, gl::texture_format::kRgba,
      gl::type_of<std::byte>(),
      nonstd::span<const std::uint8_t>{image_bytes.get(), 4 * dimensions.x * dimensions.y});
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

  gl::buffer quad_index;
  gl::texture console_font;
  gl::sampler pixel_sampler;

  impl_t(gl::program legacy_line, gl::program legacy_rect, gl::program legacy_text,
         gl::texture console_font)
  : legacy_line{std::move(legacy_line)}
  , legacy_rect{std::move(legacy_rect)}
  , legacy_text{std::move(legacy_text)}
  , quad_index(gl::make_buffer())
  , console_font{std::move(console_font)}
  , pixel_sampler{gl::make_sampler(gl::filter::kNearest, gl::filter::kNearest,
                                   gl::texture_wrap::kClampToEdge,
                                   gl::texture_wrap::kClampToEdge)} {
    static const std::vector<unsigned> quad_indices = {0, 1, 2, 0, 2, 3};
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

result<std::unique_ptr<GlRenderer>> GlRenderer::create(const io::Filesystem& fs) {
  auto console_data = fs.read_asset("console.png");
  if (!console_data) {
    return unexpected(console_data.error());
  }
  auto console_font = load_image("console.png", *console_data);
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

  auto renderer = std::make_unique<GlRenderer>(access_tag{});
  renderer->impl_ = std::make_unique<impl_t>(std::move(*legacy_line), std::move(*legacy_rect),
                                             std::move(*legacy_text), std::move(*console_font));
  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
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

void GlRenderer::render_legacy_line(const glm::vec2& a, const glm::vec2& b,
                                    const glm::vec4& colour) {
  std::array<line_t, 1> line = {{a, b, colour}};
  render_legacy_lines(line);
}

void GlRenderer::render_legacy_lines(nonstd::span<const line_t> lines) {
  const auto& program = impl_->legacy_line;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto result = gl::set_uniforms(program, "render_scale", impl_->legacy_render_scale(),
                                 "render_dimensions", impl_->render_dimensions);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }

  std::vector<float> vertex_data;
  std::vector<float> colour_data;
  std::vector<unsigned> line_indices;
  unsigned index = 0;
  for (const auto& line : lines) {
    vertex_data.emplace_back(line.a.x);
    vertex_data.emplace_back(line.a.y);
    vertex_data.emplace_back(line.b.x);
    vertex_data.emplace_back(line.b.y);
    colour_data.emplace_back(line.colour.r);
    colour_data.emplace_back(line.colour.g);
    colour_data.emplace_back(line.colour.b);
    colour_data.emplace_back(line.colour.a);
    colour_data.emplace_back(line.colour.r);
    colour_data.emplace_back(line.colour.g);
    colour_data.emplace_back(line.colour.b);
    colour_data.emplace_back(line.colour.a);
    line_indices.emplace_back(index++);
    line_indices.emplace_back(index++);
  }

  auto vertex_buffer = gl::make_buffer();
  auto colour_buffer = gl::make_buffer();
  gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw,
                  nonstd::span<const float>{vertex_data});
  gl::buffer_data(colour_buffer, gl::buffer_usage::kStreamDraw,
                  nonstd::span<const float>{colour_data});

  auto line_index = gl::make_buffer();
  gl::buffer_data(line_index, gl::buffer_usage::kStreamDraw,
                  nonstd::span<const unsigned>{line_indices});

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);
  auto position_handle = gl::vertex_float_attribute_buffer(
      vertex_buffer, 0, 2, gl::type_of<float>(), false, 2 * sizeof(float), 0);
  auto colour_handle = gl::vertex_float_attribute_buffer(colour_buffer, 1, 4, gl::type_of<float>(),
                                                         false, 4 * sizeof(float), 0);

  gl::draw_elements(gl::draw_mode::kLines, line_index, gl::type_of<unsigned>(), 2 * lines.size(),
                    0);
}

}  // namespace ii::render