#include "game/render/gl_renderer.h"
#include "assets/fonts/fonts.h"
#include "game/common/math.h"
#include "game/common/raw_ptr.h"
#include "game/io/font/font.h"
#include "game/render/font_cache.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include "game/render/render_common.h"
#include "game/render/shader_compiler.h"
#include <GL/gl3w.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <vector>

namespace ii::render {
namespace {
enum class shader {
  kText,
  kShapeOutline,
  kShapeMotion,
};

enum shape_shader_style : std::uint32_t {
  kStyleNgonPolygon = 0,
  kStyleNgonPolystar = 1,
  kStyleNgonPolygram = 2,
  kStyleBox = 3,
  kStyleLine = 4,
};

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

  FontCache font_cache;
  std::unordered_map<render::shader, gl::program> shader_map;
  gl::sampler pixel_sampler;

  impl_t()
  : pixel_sampler{gl::make_sampler(gl::filter::kNearest, gl::filter::kNearest,
                                   gl::texture_wrap::kClampToEdge,
                                   gl::texture_wrap::kClampToEdge)} {}

  const gl::program& shader(render::shader s) const {
    return shader_map.find(s)->second;
  }
};

result<std::unique_ptr<GlRenderer>> GlRenderer::create(std::uint32_t shader_version) {
  auto renderer = std::make_unique<GlRenderer>(access_tag{});
  renderer->impl_ = std::make_unique<impl_t>();

  auto load_shader =
      [&](shader s,
          const std::unordered_map<std::string, gl::shader_type>& shaders) -> result<void> {
    auto handle = compile_program(shader_version, shaders);
    if (!handle) {
      return unexpected(handle.error());
    }
    renderer->impl_->shader_map.emplace(s, std::move(*handle));
    return {};
  };

  auto r = load_shader(shader::kText,
                       {{"game/render/shaders/text.f.glsl", gl::shader_type::kFragment},
                        {"game/render/shaders/text.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  r = load_shader(shader::kShapeOutline,
                  {{"game/render/shaders/shape/colour.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/outline.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kShapeMotion,
                  {{"game/render/shaders/shape/colour.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/motion.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  r = renderer->impl_->font_cache.assign(0u, assets::fonts::roboto_mono_regular_ttf());
  if (!r) {
    return unexpected("Error loading font roboto_mono_regular: " + r.error());
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
  gl::clear_depth(0.);
  gl::clear(gl::clear_mask::kColourBufferBit | gl::clear_mask::kDepthBufferBit |
            gl::clear_mask::kStencilBufferBit);
}

void GlRenderer::set_dimensions(const glm::uvec2& screen_dimensions,
                                const glm::uvec2& render_dimensions) {
  impl_->screen_dimensions = screen_dimensions;
  impl_->render_dimensions = render_dimensions;
  impl_->font_cache.set_dimensions(screen_dimensions, render_dimensions);
}

std::int32_t GlRenderer::text_width(std::uint32_t font_index, const glm::uvec2& font_dimensions,
                                    ustring_view s) {
  auto font_result = impl_->font_cache.get(font_index, font_dimensions, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return static_cast<std::int32_t>(font_dimensions.x);
  }
  const auto& font_entry = **font_result;
  auto width = font_entry.font.calculate_width(s);
  return static_cast<std::int32_t>(
      std::ceil(static_cast<float>(width) /
                render_scale(impl_->screen_dimensions, impl_->render_dimensions)));
}

void GlRenderer::render_text(std::uint32_t font_index, const glm::uvec2& font_dimensions,
                             const glm::ivec2& position, const glm::vec4& colour, ustring_view s) {
  auto font_result = impl_->font_cache.get(font_index, font_dimensions, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return;
  }
  const auto& font_entry = **font_result;

  const auto& program = impl_->shader(shader::kText);
  gl::use_program(program);
  gl::enable_blend(true);
  gl::enable_depth_test(false);
  if (font_entry.font.is_lcd()) {
    gl::blend_function(gl::blend_factor::kSrc1Colour, gl::blend_factor::kOneMinusSrc1Colour);
  } else {
    gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  }

  auto render_scale = render_aspect_scale(impl_->screen_dimensions, impl_->render_dimensions);
  auto result =
      gl::set_uniforms(program, "render_scale", render_scale, "render_dimensions",
                       impl_->render_dimensions, "screen_dimensions", impl_->screen_dimensions,
                       "texture_dimensions", font_entry.font.bitmap_dimensions(), "is_font_lcd",
                       font_entry.font.is_lcd() ? 1u : 0u, "text_origin", glm::vec2{position},
                       "text_colour", colour, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
    return;
  }
  result = gl::set_uniform_texture_2d(program, "font_texture", /* texture unit */ 0,
                                      font_entry.texture, impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected("OpenGL error: " + result.error());
    return;
  }

  const auto vertex_data = font_entry.font.generate_vertex_data(s, {0, 0});
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

void GlRenderer::render_shapes(std::span<const shape> shapes, float trail_alpha) {
  struct vertex_data {
    std::uint32_t style = 0;
    glm::uvec2 params{0u, 0u};
    float rotation = 0.f;
    float line_width = 0.f;
    float z_index = 0.f;
    glm::vec2 position{0.f};
    glm::vec2 dimensions{0.f};
    glm::vec4 colour{0.f};
  };

  // TODO: these vectors are gl::buffers below should probably be saved between frames?
  std::vector<float> float_data;
  std::vector<unsigned> int_data;
  std::vector<unsigned> trail_indices;
  std::vector<unsigned> outline_indices;

  static constexpr std::size_t kFloatStride = 11 * sizeof(float);
  static constexpr std::size_t kIntStride = 3 * sizeof(unsigned);
  auto add_vertex_data = [&](const vertex_data& d) {
    int_data.emplace_back(d.style);
    int_data.emplace_back(d.params.x);
    int_data.emplace_back(d.params.y);
    float_data.emplace_back(d.rotation);
    float_data.emplace_back(d.line_width);
    float_data.emplace_back(d.position.x);
    float_data.emplace_back(d.position.y);
    float_data.emplace_back(d.z_index);
    float_data.emplace_back(d.dimensions.x);
    float_data.emplace_back(d.dimensions.y);
    float_data.emplace_back(d.colour.r);
    float_data.emplace_back(d.colour.g);
    float_data.emplace_back(d.colour.b);
    float_data.emplace_back(d.colour.a);
  };

  std::uint32_t vertex_index = 0;
  auto add_shape_data = [&](const vertex_data& d,
                            const std::optional<render::motion_trail>& trail) {
    add_vertex_data(d);
    outline_indices.emplace_back(vertex_index++);
    if (trail && trail_alpha > 0.f) {
      auto dt = d;
      dt.position = trail->prev_origin;
      dt.rotation = trail->prev_rotation;
      dt.colour = trail->prev_colour;
      add_vertex_data(dt);
      trail_indices.emplace_back(vertex_index - 1);
      trail_indices.emplace_back(vertex_index++);
    }
  };

  // TODO: arranging so that output is sorted by style (main switch in shader)
  // could perhaps be good for performance.
  for (const auto& shape : shapes) {
    if (const auto* p = std::get_if<render::ngon>(&shape.data)) {
      auto add_polygon = [&](std::uint32_t style, std::uint32_t param) {
        add_shape_data(
            vertex_data{
                .style = style,
                .params = {p->sides, param},
                .rotation = shape.rotation,
                .line_width = p->line_width,
                .z_index = shape.z_index.value_or(0.f),
                .position = shape.origin,
                .dimensions = {p->radius, 0.f},
                .colour = shape.colour,
            },
            shape.trail);
      };
      if (p->style != ngon_style::kPolygram || p->sides <= 3) {
        add_polygon(p->style == ngon_style::kPolystar ? kStyleNgonPolystar : kStyleNgonPolygon,
                    p->sides);
      } else if (p->sides == 4) {
        add_polygon(kStyleNgonPolygon, p->sides);
        add_polygon(kStyleNgonPolystar, p->sides);
      } else {
        add_polygon(kStyleNgonPolygon, p->sides);
        for (std::uint32_t i = 0; i + 2 < p->sides; ++i) {
          add_polygon(kStyleNgonPolygram, i);
        }
      }
    } else if (const auto* p = std::get_if<render::polyarc>(&shape.data)) {
      add_shape_data(
          vertex_data{
              .style = kStyleNgonPolygon,
              .params = {p->sides, p->segments},
              .rotation = shape.rotation,
              .line_width = p->line_width,
              .z_index = shape.z_index.value_or(0.f),
              .position = shape.origin,
              .dimensions = {p->radius, 0},
              .colour = shape.colour,
          },
          shape.trail);
    } else if (const auto* p = std::get_if<render::box>(&shape.data)) {
      add_shape_data(
          vertex_data{
              .style = kStyleBox,
              .params = {0, 0},
              .rotation = shape.rotation,
              .line_width = p->line_width,
              .z_index = shape.z_index.value_or(0.f),
              .position = shape.origin,
              .dimensions = p->dimensions,
              .colour = shape.colour,
          },
          shape.trail);
    } else if (const auto* p = std::get_if<render::line>(&shape.data)) {
      add_shape_data(
          vertex_data{
              .style = kStyleLine,
              .params = {p->sides, 0},
              .rotation = shape.rotation,
              .line_width = p->line_width,
              .z_index = shape.z_index.value_or(0.f),
              .position = shape.origin,
              .dimensions = {p->radius, 0},
              .colour = shape.colour,
          },
          shape.trail);
    }
  }

  auto float_buffer = gl::make_buffer();
  auto int_buffer = gl::make_buffer();
  auto trail_index_buffer = gl::make_buffer();
  auto outline_index_buffer = gl::make_buffer();
  gl::buffer_data(float_buffer, gl::buffer_usage::kStreamDraw, std::span<const float>{float_data});
  gl::buffer_data(int_buffer, gl::buffer_usage::kStreamDraw, std::span<const unsigned>{int_data});
  gl::buffer_data(trail_index_buffer, gl::buffer_usage::kStreamDraw,
                  std::span<const unsigned>{trail_indices});
  gl::buffer_data(outline_index_buffer, gl::buffer_usage::kStreamDraw,
                  std::span<const unsigned>{outline_indices});

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  std::uint32_t index = 0;
  std::size_t float_offset = 0;
  std::size_t int_offset = 0;
  auto add_float_attribute = [&](std::uint8_t count) {
    auto h = gl::vertex_float_attribute_buffer(float_buffer, index, count, gl::type_of<float>(),
                                               false, kFloatStride, float_offset * sizeof(float));
    ++index;
    float_offset += count;
    return h;
  };
  auto add_int_attribute = [&](std::uint8_t count) {
    auto h = gl::vertex_int_attribute_buffer(int_buffer, index, count, gl::type_of<unsigned>(),
                                             kIntStride, int_offset * sizeof(unsigned));
    ++index;
    int_offset += count;
    return h;
  };

  auto style = add_int_attribute(1);
  auto params = add_int_attribute(2);
  auto rotation = add_float_attribute(1);
  auto line_width = add_float_attribute(1);
  auto position = add_float_attribute(3);
  auto dimensions = add_float_attribute(2);
  auto colour = add_float_attribute(4);

  auto render_scale = render_aspect_scale(impl_->screen_dimensions, impl_->render_dimensions);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  if (!trail_indices.empty()) {
    const auto& motion_program = impl_->shader(shader::kShapeMotion);
    gl::use_program(motion_program);
    gl::enable_depth_test(false);

    auto result = gl::set_uniforms(motion_program, "render_scale", render_scale,
                                   "render_dimensions", impl_->render_dimensions, "trail_alpha",
                                   trail_alpha, "colour_cycle", colour_cycle_ / 256.f);
    if (!result) {
      impl_->status = unexpected(result.error());
    }
    gl::draw_elements(gl::draw_mode::kLines, trail_index_buffer, gl::type_of<unsigned>(),
                      trail_indices.size(), 0);
  }

  const auto& outline_program = impl_->shader(shader::kShapeOutline);
  gl::use_program(outline_program);
  gl::enable_depth_test(true);
  gl::depth_function(gl::comparison::kGreaterEqual);

  auto result = gl::set_uniforms(outline_program, "render_scale", render_scale, "render_dimensions",
                                 impl_->render_dimensions, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
  }
  gl::draw_elements(gl::draw_mode::kPoints, outline_index_buffer, gl::type_of<unsigned>(),
                    outline_indices.size(), 0);
}

}  // namespace ii::render
