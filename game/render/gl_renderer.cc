#include "game/render/gl_renderer.h"
#include "assets/fonts/fonts.h"
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/common/raw_ptr.h"
#include "game/io/font/font.h"
#include "game/render/font_cache.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include "game/render/noise_generator.h"
#include "game/render/render_common.h"
#include "game/render/shader_compiler.h"
#include <GL/gl3w.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ii::render {
namespace {
enum class shader {
  kText,
  kPanel,
  kBackground,
  kShapeOutline,
  kShapeMotion,
  kShapeFill,
  kPostprocess,
};

enum shape_shader_style : std::uint32_t {
  kStyleNgonPolygon = 0,
  kStyleNgonPolystar = 1,
  kStyleNgonPolygram = 2,
  kStyleBox = 3,
  kStyleLine = 4,
};

template <typename T>
gl::buffer make_stream_draw_buffer(std::span<const T> data) {
  auto buffer = gl::make_buffer();
  gl::buffer_data(buffer, gl::buffer_usage::kStreamDraw, data);
  return buffer;
}

struct framebuffer_data {
  glm::uvec2 dimensions;
  gl::framebuffer fbo;
  gl::texture colour_buffer;
  std::optional<gl::renderbuffer> depth_stencil_buffer;
};

result<framebuffer_data>
make_render_framebuffer(const glm::uvec2& dimensions, bool depth_stencil, std::uint32_t samples) {
  auto fbo = gl::make_framebuffer();
  auto colour_buffer = gl::make_texture();
  std::optional<gl::renderbuffer> depth_stencil_buffer;
  if (depth_stencil) {
    depth_stencil_buffer = gl::make_renderbuffer();
  }

  if (samples <= 1) {
    gl::texture_storage_2d(colour_buffer, 1, dimensions, gl::internal_format::kRgb32F);
    gl::framebuffer_colour_texture_2d(fbo, colour_buffer);
    if (depth_stencil) {
      gl::renderbuffer_storage(*depth_stencil_buffer, gl::internal_format::kDepth24Stencil8,
                               dimensions);
      gl::framebuffer_renderbuffer_depth_stencil(fbo, *depth_stencil_buffer);
    }
  } else {
    gl::texture_storage_2d_multisample(colour_buffer, samples, dimensions,
                                       gl::internal_format::kRgb32F);
    gl::framebuffer_colour_texture_2d_multisample(fbo, colour_buffer);
    if (depth_stencil) {
      gl::renderbuffer_storage_multisample(*depth_stencil_buffer, samples,
                                           gl::internal_format::kDepth24Stencil8, dimensions);
      gl::framebuffer_renderbuffer_depth_stencil(fbo, *depth_stencil_buffer);
    }
  }
  if (!gl::is_framebuffer_complete(fbo)) {
    return unexpected("framebuffer is not complete");
  }
  return {{dimensions, std::move(fbo), std::move(colour_buffer), std::move(depth_stencil_buffer)}};
}

struct vertex_attribute_container {
  struct type_info {
    gl::buffer buffer;
    std::size_t total_count_per_vertex = 0;
    std::size_t current_offset = 0;
  };
  std::unordered_map<gl::type, type_info> buffers;
  std::vector<gl::vertex_attribute> attributes;

  template <typename T>
  void add_buffer(std::span<const T> data, std::size_t total_count_per_vertex) {
    auto buffer = make_stream_draw_buffer(data);
    buffers.emplace(
        gl::type_of<T>(),
        type_info{.buffer = std::move(buffer), .total_count_per_vertex = total_count_per_vertex});
  }

  template <typename T>
  void add_attribute(std::size_t index, std::size_t count_per_vertex) {
    auto it = buffers.find(gl::type_of<T>());
    assert(it != buffers.end());
    auto& info = it->second;
    if constexpr (std::is_integral_v<T>) {
      attributes.emplace_back(gl::vertex_int_attribute_buffer(
          info.buffer, index, count_per_vertex, gl::type_of<T>(),
          info.total_count_per_vertex * sizeof(T), info.current_offset * sizeof(T)));
    } else {
      attributes.emplace_back(gl::vertex_float_attribute_buffer(
          info.buffer, index, count_per_vertex, gl::type_of<T>(),
          /* normalize */ false, info.total_count_per_vertex * sizeof(T),
          info.current_offset * sizeof(T)));
    }
    info.current_offset += count_per_vertex;
  }
};
}  // namespace

// TODO: general things:
// - can vertex array objects be saved between calls?
// - can we reuse index buffers or any other buffers?
struct GlRenderer::impl_t {
  result<void> status;

  FontCache font_cache;
  std::unordered_map<render::shader, gl::program> shader_map;
  gl::sampler pixel_sampler;
  gl::sampler linear_sampler;

  std::optional<gl::buffer> iota_index_buffer;
  std::uint32_t iota_index_size = 0;

  std::optional<framebuffer_data> render_framebuffer;
  std::optional<framebuffer_data> postprocess_framebuffer;

  impl_t()
  : pixel_sampler{gl::make_sampler(gl::filter::kNearest, gl::filter::kNearest,
                                   gl::texture_wrap::kClampToEdge, gl::texture_wrap::kClampToEdge)}
  , linear_sampler{gl::make_sampler(gl::filter::kLinear, gl::filter::kLinear,
                                    gl::texture_wrap::kRepeat, gl::texture_wrap::kRepeat)} {}

  const gl::program& shader(render::shader s) const { return shader_map.find(s)->second; }

  const gl::buffer& index_buffer(std::uint32_t indices) {
    if (!iota_index_buffer) {
      iota_index_buffer = gl::make_buffer();
    }
    if (indices > iota_index_size) {
      std::vector<std::uint32_t> data;
      for (std::uint32_t i = 0; i < indices; ++i) {
        data.emplace_back(i);
      }
      gl::buffer_data(*iota_index_buffer, gl::buffer_usage::kStaticDraw,
                      std::span<const std::uint32_t>{data});
      iota_index_size = indices;
    }
    return *iota_index_buffer;
  }

  gl::bound_framebuffer bind_render_framebuffer(const glm::uvec2& dimensions) {
    if (!render_framebuffer || render_framebuffer->dimensions != dimensions) {
      auto result = make_render_framebuffer(dimensions, /* depth/stencil */ true, /* samples */ 16);
      if (!result) {
        status = unexpected("OpenGL error: " + result.error());
        return gl::bound_framebuffer{GL_FRAMEBUFFER};
      }
      render_framebuffer = std::move(*result);
      result = make_render_framebuffer(dimensions, /* depth/stencil */ false, /* samples */ 1);
      if (!result) {
        status = unexpected("OpenGL error: " + result.error());
      }
      postprocess_framebuffer = std::move(*result);
    }
    return gl::bind_draw_framebuffer(render_framebuffer->fbo);
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
                       {{"game/render/shaders/ui/text.f.glsl", gl::shader_type::kFragment},
                        {"game/render/shaders/ui/text.g.glsl", gl::shader_type::kGeometry},
                        {"game/render/shaders/ui/text.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  r = load_shader(shader::kPanel,
                  {{"game/render/shaders/ui/panel.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/ui/panel.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/ui/panel.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  r = load_shader(shader::kBackground,
                  {{"game/render/shaders/bg/background.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/bg/background.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/bg/background.v.glsl", gl::shader_type::kVertex}});
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
  r = load_shader(shader::kShapeFill,
                  {{"game/render/shaders/shape/colour.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/fill.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kPostprocess,
                  {{"game/render/shaders/post/postprocess.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/post/postprocess.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  auto load_font = [&](font_id id, const std::string& name,
                       std::span<const std::uint8_t> data) -> result<void> {
    auto r = renderer->impl_->font_cache.assign(id, data);
    if (!r) {
      return unexpected("Error loading font " + name + ": " + r.error());
    }
    return {};
  };
  if (auto r = load_font(font_id::kMonospace, "roboto_mono_regular",
                         assets::fonts::roboto_mono_regular_ttf());
      !r) {
    return unexpected(r.error());
  }
  if (auto r = load_font(font_id::kMonospaceBold, "roboto_mono_bold",
                         assets::fonts::roboto_mono_bold_ttf());
      !r) {
    return unexpected(r.error());
  }
  if (auto r = load_font(font_id::kMonospaceItalic, "roboto_mono_italic",
                         assets::fonts::roboto_mono_italic_ttf());
      !r) {
    return unexpected(r.error());
  }
  if (auto r = load_font(font_id::kMonospaceBoldItalic, "roboto_mono_bold_italic",
                         assets::fonts::roboto_mono_bold_italic_ttf());
      !r) {
    return unexpected(r.error());
  }
  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
GlRenderer::~GlRenderer() = default;

result<void> GlRenderer::status() const {
  return impl_->status;
}

void GlRenderer::clear_screen() const {
  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  gl::clear_colour({0.f, 0.f, 0.f, 0.f});
  gl::clear_depth(0.);
  gl::clear(gl::clear_mask::kColourBufferBit | gl::clear_mask::kDepthBufferBit |
            gl::clear_mask::kStencilBufferBit);
}

void GlRenderer::clear_depth() const {
  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  gl::clear_depth(0.);
  gl::clear(gl::clear_mask::kDepthBufferBit);
}

std::int32_t GlRenderer::line_height(font_id font, const glm::uvec2& font_dimensions) const {
  auto font_result = impl_->font_cache.get(target(), font, font_dimensions, ustring::ascii(""));
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return 0;
  }
  const auto& font_entry = **font_result;
  auto height = font_entry.font.line_height();
  return static_cast<std::int32_t>(std::ceil(static_cast<float>(height) / target().scale_factor()));
}

std::int32_t
GlRenderer::text_width(font_id font, const glm::uvec2& font_dimensions, ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, font_dimensions, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return 0;
  }
  const auto& font_entry = **font_result;
  auto width = font_entry.font.calculate_width(s);
  return static_cast<std::int32_t>(std::ceil(static_cast<float>(width) / target().scale_factor()));
}

ustring GlRenderer::trim_for_width(font_id font, const glm::uvec2& font_dimensions,
                                   std::int32_t width, ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, font_dimensions, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return ustring::ascii("");
  }
  const auto& font_entry = **font_result;
  auto screen_width =
      static_cast<std::int32_t>(std::floor(static_cast<float>(width) * target().scale_factor()));
  return font_entry.font.trim_for_width(s, screen_width);
}

void GlRenderer::render_text(font_id font, const glm::uvec2& font_dimensions,
                             const glm::ivec2& position, const glm::vec4& colour, bool clip,
                             ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, font_dimensions, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return;
  }
  const auto& font_entry = **font_result;

  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  const auto& program = impl_->shader(shader::kText);
  gl::use_program(program);
  if (clip) {
    gl::enable_clip_planes(4u);
  } else {
    gl::enable_clip_planes(0u);
  }
  gl::enable_blend(true);
  gl::enable_depth_test(false);
  if (font_entry.font.is_lcd()) {
    gl::blend_function(gl::blend_factor::kSrc1Colour, gl::blend_factor::kOneMinusSrc1Colour);
  } else {
    gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  }

  auto clip_rect = target().clip_rect();
  auto text_origin = target().render_to_screen_coords(position + clip_rect.min());
  auto result = gl::set_uniforms(program, "screen_dimensions", target().screen_dimensions,
                                 "texture_dimensions", font_entry.font.bitmap_dimensions(),
                                 "clip_min", target().render_to_screen_coords(clip_rect.min()),
                                 "clip_max", target().render_to_screen_coords(clip_rect.max()),
                                 "is_font_lcd", font_entry.font.is_lcd() ? 1u : 0u, "text_colour",
                                 colour, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
  }
  result = gl::set_uniform_texture_2d(program, "font_texture", /* texture unit */ 0,
                                      font_entry.texture, impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected("text shader error: " + result.error());
    return;
  }

  std::vector<std::int16_t> vertex_data;
  std::uint32_t vertices = 0;
  font_entry.font.iterate_glyph_data(
      s, text_origin,
      [&](const glm::ivec2& position, const glm::ivec2& texture_coords,
          const glm::ivec2& dimensions) {
        vertex_data.emplace_back(static_cast<std::int16_t>(position.x));
        vertex_data.emplace_back(static_cast<std::int16_t>(position.y));
        vertex_data.emplace_back(static_cast<std::int16_t>(dimensions.x));
        vertex_data.emplace_back(static_cast<std::int16_t>(dimensions.y));
        vertex_data.emplace_back(static_cast<std::int16_t>(texture_coords.x));
        vertex_data.emplace_back(static_cast<std::int16_t>(texture_coords.y));
        ++vertices;
      });

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const std::int16_t>(vertex_data), 6);

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* texture_coords */ 2, 2);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(vertices),
                    gl::type_of<std::uint32_t>(), vertices, 0);
}

void GlRenderer::render_panel(const panel_data& p) const {
  if (p.style == panel_style::kNone) {
    return;
  }

  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  const auto& program = impl_->shader(shader::kPanel);
  gl::use_program(program);
  gl::enable_clip_planes(4u);
  gl::enable_blend(true);
  gl::enable_depth_test(false);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto clip_rect = target().clip_rect();
  auto result = gl::set_uniforms(program, "screen_dimensions", target().screen_dimensions,
                                 "clip_min", target().render_to_screen_coords(clip_rect.min()),
                                 "clip_max", target().render_to_screen_coords(clip_rect.max()),
                                 "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected("panel shader error: " + result.error());
    return;
  }

  auto min = target().render_to_screen_coords(clip_rect.min() + p.bounds.min());
  auto size = target().render_to_screen_coords(clip_rect.min() + p.bounds.max()) - min;

  std::vector<float> float_data = {p.colour.r, p.colour.g, p.colour.b, p.colour.a};
  std::vector<std::int16_t> int_data = {
      static_cast<std::int16_t>(min.x),           static_cast<std::int16_t>(min.y),
      static_cast<std::int16_t>(size.x),          static_cast<std::int16_t>(size.y),
      static_cast<std::int16_t>(p.bounds.size.x), static_cast<std::int16_t>(p.bounds.size.y),
      static_cast<std::int16_t>(p.style)};

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const float>{float_data}, 4);
  attributes.add_buffer(std::span<const std::int16_t>{int_data}, 7);

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* screen_dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* render_dimensions */ 2, 2);
  attributes.add_attribute<float>(/* colour */ 3, 4);
  attributes.add_attribute<std::int16_t>(/* style */ 4, 1);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(1), gl::type_of<unsigned>(), 1, 0);
}

void GlRenderer::render_background(std::uint64_t tick_count, const glm::vec4& colour) const {
  // TODO: move this stuff out into overmind.
  auto t = static_cast<float>(tick_count);
  glm::vec4 offset{256.f * std::cos(t / 512.f), -t / 4.f, t / 8.f, t / 256.f};
  glm::vec2 parameters{(1.f - std::cos(t / 1024.f)) / 2.f, 0.f};

  // TODO: background shader is way more complicated than it needs to be to draw a quad.
  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  const auto& program = impl_->shader(shader::kBackground);
  gl::use_program(program);
  gl::enable_clip_planes(4u);
  gl::enable_blend(false);
  gl::enable_depth_test(false);

  auto clip_rect = target().clip_rect();
  auto result =
      gl::set_uniforms(program, "screen_dimensions", target().screen_dimensions, "clip_min",
                       target().render_to_screen_coords(clip_rect.min()), "clip_max",
                       target().render_to_screen_coords(clip_rect.max()), "offset", offset,
                       "parameters", parameters, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected("background shader error: " + result.error());
    return;
  }

  auto min = target().render_to_screen_coords(clip_rect.min());
  auto size = target().render_to_screen_coords(clip_rect.max()) - min;

  std::vector<float> float_data = {colour.r, colour.g, colour.b, colour.a};
  std::vector<std::int16_t> int_data = {static_cast<std::int16_t>(min.x),
                                        static_cast<std::int16_t>(min.y),
                                        static_cast<std::int16_t>(size.x),
                                        static_cast<std::int16_t>(size.y),
                                        static_cast<std::int16_t>(clip_rect.size.x),
                                        static_cast<std::int16_t>(clip_rect.size.y)};

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const float>{float_data}, 4);
  attributes.add_buffer(std::span<const std::int16_t>{int_data}, 7);

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* screen_dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* render_dimensions */ 2, 2);
  attributes.add_attribute<float>(/* colour */ 3, 4);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(1), gl::type_of<unsigned>(), 1, 0);
}

void GlRenderer::render_shapes(coordinate_system ctype, std::vector<shape>& shapes,
                               shape_style style, float trail_alpha) const {
  std::stable_sort(shapes.begin(), shapes.end(),
                   [](const shape& a, const shape& b) { return a.z_index < b.z_index; });

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

  // TODO: these vectors and gl::buffers below should probably be saved between frames?
  static thread_local std::vector<float> float_data;
  static thread_local std::vector<std::uint8_t> int_data;
  static thread_local std::vector<unsigned> shadow_trail_indices;
  static thread_local std::vector<unsigned> shadow_fill_indices;
  static thread_local std::vector<unsigned> shadow_outline_indices;
  static thread_local std::vector<unsigned> bottom_outline_indices;
  static thread_local std::vector<unsigned> trail_indices;
  static thread_local std::vector<unsigned> fill_indices;
  static thread_local std::vector<unsigned> outline_indices;
  float_data.clear();
  int_data.clear();
  shadow_trail_indices.clear();
  shadow_fill_indices.clear();
  shadow_outline_indices.clear();
  bottom_outline_indices.clear();
  trail_indices.clear();
  fill_indices.clear();
  outline_indices.clear();

  auto add_vertex_data = [&](const vertex_data& d) {
    int_data.emplace_back(static_cast<std::uint8_t>(d.style));
    int_data.emplace_back(static_cast<std::uint8_t>(d.params.x));
    int_data.emplace_back(static_cast<std::uint8_t>(d.params.y));
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

  static constexpr glm::vec2 kShadowOffset{4, 6};
  std::uint32_t vertex_index = 0;
  auto add_outline_data = [&](const vertex_data& d,
                              const std::optional<render::motion_trail>& trail) {
    add_vertex_data(d);
    if (d.z_index < colour::kZTrails) {
      bottom_outline_indices.emplace_back(vertex_index++);
    } else {
      outline_indices.emplace_back(vertex_index++);
    }
    if (trail && trail_alpha > 0.f) {
      auto dt = d;
      dt.position = trail->prev_origin;
      dt.rotation = trail->prev_rotation;
      dt.colour = trail->prev_colour;
      add_vertex_data(dt);
      trail_indices.emplace_back(vertex_index - 1);
      trail_indices.emplace_back(vertex_index++);
    }
    if (style == shape_style::kStandard) {
      auto ds = d;
      ds.position += kShadowOffset;
      ds.colour = glm::vec4{0.f, 0.f, 0.f, ds.colour.a / 2.f};
      ds.line_width += 1.f;
      add_vertex_data(ds);
      shadow_outline_indices.emplace_back(vertex_index++);
      if (trail && trail_alpha > 0.f) {
        auto dt = ds;
        dt.position = trail->prev_origin + kShadowOffset;
        dt.rotation = trail->prev_rotation;
        dt.colour = {0.f, 0.f, 0.f, trail->prev_colour.a / 2.f};
        add_vertex_data(dt);
        shadow_trail_indices.emplace_back(vertex_index - 1);
        shadow_trail_indices.emplace_back(vertex_index++);
      }
    }
  };

  auto add_fill_data = [&](const vertex_data& d) {
    add_vertex_data(d);
    fill_indices.emplace_back(vertex_index++);
    if (style == shape_style::kStandard) {
      auto ds = d;
      ds.position += kShadowOffset;
      ds.colour = {0.f, 0.f, 0.f, ds.colour.a / 2.f};
      add_vertex_data(ds);
      shadow_fill_indices.emplace_back(vertex_index++);
    }
  };

  // TODO: arranging so that output is sorted by style (main switch in shader)
  // could perhaps be good for performance.
  // NOTE: due to geometry shader hardware limits and current implementation,
  // maximum sides for shapes are:
  // - polygon:  41
  // - polystar: 28 even, 17 odd
  // - polygram: 17
  // Limits could be raised by avoiding clipping in geometry shader, or by sending
  // more input vertices.
  for (const auto& shape : shapes) {
    if (const auto* p = std::get_if<render::ngon>(&shape.data)) {
      auto add_polygon = [&](std::uint32_t style, std::uint32_t param) {
        add_outline_data(
            {
                .style = style,
                .params = {p->sides, param},
                .rotation = shape.rotation,
                .line_width = p->line_width,
                .z_index = shape.z_index,
                .position = shape.origin,
                .dimensions = {p->radius, p->inner_radius},
                .colour = shape.colour,
            },
            shape.trail);
      };
      if (p->style != ngon_style::kPolygram || p->sides <= 3) {
        add_polygon(p->style == ngon_style::kPolystar ? kStyleNgonPolystar : kStyleNgonPolygon,
                    p->segments);
      } else if (p->sides == 4) {
        add_polygon(kStyleNgonPolygon, p->segments);
        add_polygon(kStyleNgonPolystar, p->segments);
      } else {
        add_polygon(kStyleNgonPolygon, p->segments);
        for (std::uint32_t i = 0; i + 2 < p->sides; ++i) {
          add_polygon(kStyleNgonPolygram, i);
        }
      }
    } else if (const auto* p = std::get_if<render::box>(&shape.data)) {
      add_outline_data(
          {
              .style = kStyleBox,
              .params = {0, 0},
              .rotation = shape.rotation,
              .line_width = p->line_width,
              .z_index = shape.z_index,
              .position = shape.origin,
              .dimensions = p->dimensions,
              .colour = shape.colour,
          },
          shape.trail);
    } else if (const auto* p = std::get_if<render::line>(&shape.data)) {
      add_outline_data(
          {
              .style = kStyleLine,
              .params = {p->sides, 0},
              .rotation = shape.rotation,
              .line_width = p->line_width,
              .z_index = shape.z_index,
              .position = shape.origin,
              .dimensions = {p->radius, 0},
              .colour = shape.colour,
          },
          shape.trail);
    } else if (const auto* p = std::get_if<render::ngon_fill>(&shape.data)) {
      add_fill_data({
          .style = kStyleNgonPolygon,
          .params = {p->sides, p->segments},
          .rotation = shape.rotation,
          .z_index = shape.z_index,
          .position = shape.origin,
          .dimensions = {p->radius, p->inner_radius},
          .colour = shape.colour,
      });
    } else if (const auto* p = std::get_if<render::box_fill>(&shape.data)) {
      add_fill_data({
          .style = kStyleBox,
          .params = {0, 0},
          .rotation = shape.rotation,
          .z_index = shape.z_index,
          .position = shape.origin,
          .dimensions = p->dimensions,
          .colour = shape.colour,
      });
    }
  }

  auto shadow_trail_index_buffer =
      make_stream_draw_buffer(std::span<const unsigned>{shadow_trail_indices});
  auto shadow_fill_index_buffer =
      make_stream_draw_buffer(std::span<const unsigned>{shadow_fill_indices});
  auto shadow_outline_index_buffer =
      make_stream_draw_buffer(std::span<const unsigned>{shadow_outline_indices});
  auto bottom_outline_index_buffer =
      make_stream_draw_buffer(std::span<const unsigned>{bottom_outline_indices});
  auto trail_index_buffer = make_stream_draw_buffer(std::span<const unsigned>{trail_indices});
  auto fill_index_buffer = make_stream_draw_buffer(std::span<const unsigned>{fill_indices});
  auto outline_index_buffer = make_stream_draw_buffer(std::span<const unsigned>{outline_indices});
  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const std::uint8_t>(int_data), 3);
  attributes.add_buffer(std::span<const float>{float_data}, 11);

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  attributes.add_attribute<std::uint8_t>(/* style */ 0, 1);
  attributes.add_attribute<std::uint8_t>(/* params */ 1, 2);
  attributes.add_attribute<float>(/* rotation */ 2, 1);
  attributes.add_attribute<float>(/* line_width */ 3, 1);
  attributes.add_attribute<float>(/* position */ 4, 3);
  attributes.add_attribute<float>(/* dimensions */ 5, 2);
  attributes.add_attribute<float>(/* colour */ 6, 4);

  auto clip_rect = target().clip_rect();
  glm::vec2 offset;
  switch (ctype) {
  case coordinate_system::kGlobal:
    offset = glm::vec2{0, 0};
    break;
  case coordinate_system::kLocal:
    offset = clip_rect.min();
    break;
  case coordinate_system::kCentered:
    offset = (clip_rect.min() + clip_rect.max()) / 2;
  }

  auto fbo_bind = impl_->bind_render_framebuffer(target_.screen_dimensions);
  gl::enable_clip_planes(4u);
  gl::enable_depth_test(false);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto render_outlines = [&](const gl::buffer& index_buffer, std::size_t elements) {
    const auto& outline_program = impl_->shader(shader::kShapeOutline);
    gl::use_program(outline_program);
    auto result = gl::set_uniforms(
        outline_program, "aspect_scale", target().aspect_scale(), "render_dimensions",
        target().render_dimensions, "clip_min", clip_rect.min(), "clip_max", clip_rect.max(),
        "coordinate_offset", offset, "colour_cycle", colour_cycle_ / 256.f);
    if (!result) {
      impl_->status = unexpected("outline shader error: " + result.error());
    } else {
      gl::draw_elements(gl::draw_mode::kPoints, index_buffer, gl::type_of<unsigned>(), elements, 0);
    }
  };

  auto render_trails = [&](const gl::buffer& index_buffer, std::size_t elements) {
    const auto& motion_program = impl_->shader(shader::kShapeMotion);
    gl::use_program(motion_program);
    auto result =
        gl::set_uniforms(motion_program, "aspect_scale", target().aspect_scale(),
                         "render_dimensions", target().render_dimensions, "clip_min",
                         clip_rect.min(), "clip_max", clip_rect.max(), "coordinate_offset", offset,
                         "trail_alpha", trail_alpha, "colour_cycle", colour_cycle_ / 256.f);
    if (!result) {
      impl_->status = unexpected("motion shader error: " + result.error());
    } else {
      gl::draw_elements(gl::draw_mode::kLines, index_buffer, gl::type_of<unsigned>(), elements, 0);
    }
  };

  auto render_fills = [&](const gl::buffer& index_buffer, std::size_t elements) {
    const auto& fill_program = impl_->shader(shader::kShapeFill);
    gl::use_program(fill_program);
    auto result = gl::set_uniforms(
        fill_program, "aspect_scale", target().aspect_scale(), "render_dimensions",
        target().render_dimensions, "clip_min", clip_rect.min(), "clip_max", clip_rect.max(),
        "coordinate_offset", offset, "colour_cycle", colour_cycle_ / 256.f);
    if (!result) {
      impl_->status = unexpected("fill shader error: " + result.error());
    } else {
      gl::draw_elements(gl::draw_mode::kPoints, index_buffer, gl::type_of<unsigned>(), elements, 0);
    }
  };

  if (!shadow_trail_indices.empty()) {
    render_trails(shadow_trail_index_buffer, shadow_trail_indices.size());
  }
  if (!shadow_outline_indices.empty()) {
    render_outlines(shadow_outline_index_buffer, shadow_outline_indices.size());
  }
  if (!shadow_fill_indices.empty()) {
    render_fills(shadow_fill_index_buffer, shadow_fill_indices.size());
  }
  if (!bottom_outline_indices.empty()) {
    render_outlines(bottom_outline_index_buffer, bottom_outline_indices.size());
  }
  if (!trail_indices.empty()) {
    render_trails(trail_index_buffer, trail_indices.size());
  }
  if (!fill_indices.empty()) {
    render_fills(fill_index_buffer, fill_indices.size());
  }
  if (!outline_indices.empty()) {
    render_outlines(outline_index_buffer, outline_indices.size());
  }
}

void GlRenderer::render_present() const {
  if (!impl_->render_framebuffer || !impl_->postprocess_framebuffer) {
    return;
  }
  {
    auto bind_read = gl::bind_read_framebuffer(impl_->render_framebuffer->fbo);
    auto bind_draw = gl::bind_draw_framebuffer(impl_->postprocess_framebuffer->fbo);
    gl::blit_framebuffer_colour(target_.screen_dimensions);
  }

  const auto& program = impl_->shader(shader::kPostprocess);
  gl::use_program(program);
  gl::enable_clip_planes(0u);
  gl::enable_blend(false);
  gl::enable_depth_test(false);
  gl::enable_srgb(false);

  auto result = gl::set_uniform_texture_2d(program, "framebuffer_texture", /* texture unit */ 0,
                                           impl_->postprocess_framebuffer->colour_buffer,
                                           impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected("postprocess shader error: " + result.error());
    return;
  }

  static const std::vector<float> vertex_data = {
      -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f,
  };

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const float>(vertex_data), 2);

  auto vertex_array = gl::make_vertex_array();
  gl::bind_vertex_array(vertex_array);

  attributes.add_attribute<float>(/* position */ 0, 2);
  gl::draw_elements(gl::draw_mode::kTriangles, impl_->index_buffer(6u),
                    gl::type_of<std::uint32_t>(), 6u, 0);
}

}  // namespace ii::render
