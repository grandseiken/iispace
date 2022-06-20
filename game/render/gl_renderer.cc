#include "game/render/gl_renderer.h"
#include "assets/fonts/fonts.h"
#include "game/common/raw_ptr.h"
#include "game/io/file/filesystem.h"
#include "game/io/font/font.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include "game/render/shader_compiler.h"
#include <GL/gl3w.h>
#include <algorithm>
#include <span>
#include <string>
#include <vector>

namespace ii::render {
namespace {

const std::vector<std::uint32_t>& utf32_codes() {
  static const char* chars =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789"
      " _!?\"$%^&*()[]{}!/\\@',.-=+#~;:<>£";
  static const std::vector<std::uint32_t> codes = [&] {
    std::vector<std::uint32_t> v;
    for (const char* c = chars; *c; ++c) {
      v.emplace_back(+*c);
    }
    return v;
  }();
  return codes;
}

}  // namespace

struct GlRenderer::impl_t {
  result<void> status;
  glm::uvec2 screen_dimensions{0, 0};
  glm::uvec2 render_dimensions{0, 0};

  gl::program line;
  gl::program rect;
  gl::program text;

  gl::buffer quad_index;
  gl::sampler pixel_sampler;

  struct font_entry {
    RenderedFont font;
    gl::texture texture;
  };
  std::unordered_map<std::uint32_t, font_entry> font_map;

  impl_t(gl::program line, gl::program rect, gl::program text)
  : line{std::move(line)}
  , rect{std::move(rect)}
  , text{std::move(text)}
  , quad_index(gl::make_buffer())
  , pixel_sampler{gl::make_sampler(gl::filter::kNearest, gl::filter::kNearest,
                                   gl::texture_wrap::kClampToEdge,
                                   gl::texture_wrap::kClampToEdge)} {
    static const std::vector<unsigned> quad_indices = {0, 1, 2, 0, 2, 3};
    gl::buffer_data(quad_index, gl::buffer_usage::kStaticDraw, std::span{quad_indices});
  }

  glm::vec2 render_scale() const {
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

  void draw_rect_internal(const glm::ivec2& position, const glm::ivec2& size) const {
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
    gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw, std::span{vertex_data});

    auto vertex_array = gl::make_vertex_array();
    gl::bind_vertex_array(vertex_array);
    auto position_handle = gl::vertex_int_attribute_buffer(
        vertex_buffer, 0, 2, gl::type_of<int_t>(), 4 * sizeof(int_t), 0);
    auto tex_coords_handle = gl::vertex_int_attribute_buffer(
        vertex_buffer, 1, 2, gl::type_of<int_t>(), 4 * sizeof(int_t), 2 * sizeof(int_t));
    gl::draw_elements(gl::draw_mode::kTriangles, quad_index, gl::type_of<unsigned>(), 6, 0);
  }
};

result<std::unique_ptr<GlRenderer>> GlRenderer::create() {
  auto compile_shader = [](gl::shader_type type, auto& entry) -> result<gl::shader> {
    auto shader = gl::compile_shader(type, entry.second);
    if (!shader) {
      return unexpected("Error compiling " + entry.first + ": " + shader.error());
    }
    return {std::move(shader)};
  };

  auto line = compile_program({{"game/render/shaders/line.f.glsl", gl::shader_type::kFragment},
                               {"game/render/shaders/line.v.glsl", gl::shader_type::kVertex}});
  if (!line) {
    return unexpected(line.error());
  }

  auto rect = compile_program({{"game/render/shaders/rect.f.glsl", gl::shader_type::kFragment},
                               {"game/render/shaders/rect.v.glsl", gl::shader_type::kVertex}});
  if (!rect) {
    return unexpected(rect.error());
  }

  auto text = compile_program({{"game/render/shaders/text.f.glsl", gl::shader_type::kFragment},
                               {"game/render/shaders/text.v.glsl", gl::shader_type::kVertex}});
  if (!text) {
    return unexpected(text.error());
  }

  auto renderer = std::make_unique<GlRenderer>(access_tag{});
  renderer->impl_ = std::make_unique<impl_t>(std::move(*line), std::move(*rect), std::move(*text));

  auto load_font = [&](std::uint32_t index, const Font& source, bool lcd,
                       const std::vector<std::uint32_t>& codes,
                       const glm::uvec2& dimensions) -> result<void> {
    auto rendered_font = source.render(codes, lcd, dimensions);
    if (!rendered_font) {
      return unexpected("Error rendering font: " + rendered_font.error());
    }

    auto texture = gl::make_texture();
    gl::texture_image_2d(
        texture, 0, rendered_font->is_lcd() ? gl::internal_format::kRgb : gl::internal_format::kRed,
        rendered_font->bitmap_dimensions(),
        rendered_font->is_lcd() ? gl::texture_format::kRgb : gl::texture_format::kRed,
        gl::type_of<std::byte>(), rendered_font->bitmap());

    impl_t::font_entry entry{std::move(*rendered_font), std::move(texture)};
    renderer->impl_->font_map.emplace(index, std::move(entry));
    return {};
  };

  auto font = Font::create(assets::fonts::roboto_mono_regular_ttf());
  if (!font) {
    return unexpected("Error loading font roboto_mono_regular: " + font.error());
  }
  auto result = load_font(0, *font, /* lcd */ true, utf32_codes(), {16, 16});
  if (!result) {
    return unexpected(result.error());
  }

  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
GlRenderer::~GlRenderer() = default;

result<void> GlRenderer::status() const {
  return impl_->status;
}

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

void GlRenderer::render_text(std::uint32_t font_index, const glm::ivec2& position,
                             const glm::vec4& colour, ustring_view s) {
  auto it = impl_->font_map.find(font_index);
  if (it == impl_->font_map.end()) {
    impl_->status = unexpected("Font not found");
    return;
  }
  auto& font_entry = it->second;

  const auto& program = impl_->text;
  gl::use_program(program);
  gl::enable_blend(true);
  if (font_entry.font.is_lcd()) {
    gl::blend_function(gl::blend_factor::kSrc1Colour, gl::blend_factor::kOneMinusSrc1Colour);
  } else {
    gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  }

  auto result = gl::set_uniforms(program, "render_scale", impl_->render_scale(),
                                 "render_dimensions", impl_->render_dimensions,
                                 "texture_dimensions", font_entry.font.bitmap_dimensions(),
                                 "is_font_lcd", font_entry.font.is_lcd() ? 1u : 0u, "text_colour",
                                 colour, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
  }
  result = gl::set_uniform_texture_2d(program, "font_texture", /* texture unit */ 0,
                                      font_entry.texture, impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected("OpenGL error: " + result.error());
  }

  const auto vertex_data = font_entry.font.generate_vertex_data(s, position);
  auto vertex_buffer = gl::make_buffer();
  gl::buffer_data(vertex_buffer, gl::buffer_usage::kStreamDraw, std::span{vertex_data});

  auto quads = static_cast<unsigned>(vertex_data.size() / 16);
  std::vector<unsigned> indices;
  for (unsigned i = 0; i < quads; ++i) {
    indices.insert(indices.end(), {4 * i, 4 * i + 1, 4 * i + 2, 4 * i, 4 * i + 2, 4 * i + 3});
  }
  auto index_buffer = gl::make_buffer();
  gl::buffer_data(index_buffer, gl::buffer_usage::kStreamDraw, std::span<const unsigned>{indices});

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);
  auto position_handle = gl::vertex_int_attribute_buffer(
      vertex_buffer, 0, 2, gl::type_of<std::int32_t>(), 4 * sizeof(std::int32_t), 0);
  auto tex_coords_handle =
      gl::vertex_int_attribute_buffer(vertex_buffer, 1, 2, gl::type_of<std::int32_t>(),
                                      4 * sizeof(std::int32_t), 2 * sizeof(std::int32_t));
  gl::draw_elements(gl::draw_mode::kTriangles, index_buffer, gl::type_of<unsigned>(), quads * 6, 0);
}

void GlRenderer::render_rect(const glm::ivec2& position, const glm::ivec2& size,
                             std::uint32_t border_width, const glm::vec4& colour_lo,
                             const glm::vec4& colour_hi, const glm::vec4& border_lo,
                             const glm::vec4& border_hi) {
  const auto& program = impl_->rect;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto result = gl::set_uniforms(
      program, "render_scale", impl_->render_scale(), "render_dimensions", impl_->render_dimensions,
      "rect_dimensions", static_cast<glm::uvec2>(size), "rect_colour_lo", colour_lo,
      "rect_colour_hi", colour_hi, "border_colour_lo", border_lo, "border_colour_hi", border_hi,
      "border_size", glm::uvec2{border_width, border_width});
  if (!result) {
    impl_->status = unexpected(result.error());
  }
  impl_->draw_rect_internal(position, size);
}

void GlRenderer::render_lines(std::span<const line_t> lines) {
  const auto& program = impl_->line;
  gl::use_program(program);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto result =
      gl::set_uniforms(program, "render_scale", impl_->render_scale(), "render_dimensions",
                       impl_->render_dimensions, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
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
                  std::span<const float>{vertex_data});
  gl::buffer_data(colour_buffer, gl::buffer_usage::kStreamDraw,
                  std::span<const float>{colour_data});

  auto line_index = gl::make_buffer();
  gl::buffer_data(line_index, gl::buffer_usage::kStreamDraw,
                  std::span<const unsigned>{line_indices});

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