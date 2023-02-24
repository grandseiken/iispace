#include "game/render/gl_renderer.h"
#include "assets/fonts/fonts.h"
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/common/raw_ptr.h"
#include "game/common/variant_switch.h"
#include "game/io/font/font.h"
#include "game/render/font_cache.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include "game/render/noise_generator.h"
#include "game/render/shader_compiler.h"
#include <GL/gl3w.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ii::render {
namespace {
constexpr std::array<float, 12> kQuadVertexData = {
    -1.f, -1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f,
};

enum class framebuffer {
  kRender,
  kAux0,
  kAux1,
};

enum class shader {
  kText,
  kPanel,
  kBackground,
  kFx,
  kShapeOutline,
  kShapeMotion,
  kShapeFill,
  kOklabResolve,
  kOutlineMask,
  kFullscreenBlend,
};

enum shape_shader_style : std::uint32_t {
  kStyleNgonPolygon = 0,
  kStyleNgonPolystar = 1,
  kStyleNgonPolygram = 2,
  kStyleBox = 3,
  kStyleLine = 4,
  kStyleBall = 5,
};

enum fx_shape : std::uint32_t {
  kFxShapeBall = 0,
  kFxShapeBox = 1,
};

template <typename T>
gl::buffer make_stream_draw_buffer(std::span<const T> data) {
  auto buffer = gl::make_buffer();
  gl::buffer_data(buffer, gl::buffer_usage::kStreamDraw, data);
  return buffer;
}

struct framebuffer_data {
  uvec2 dimensions{0};
  std::uint32_t samples = 1;
  gl::framebuffer fbo;
  gl::texture colour_buffer;
  std::optional<gl::renderbuffer> depth_stencil_buffer;
};

result<framebuffer_data> make_render_framebuffer(const uvec2& dimensions, bool alpha,
                                                 bool depth_stencil, std::uint32_t samples) {
  auto fbo = gl::make_framebuffer();
  auto colour_buffer = gl::make_texture();
  std::optional<gl::renderbuffer> depth_stencil_buffer;
  if (depth_stencil) {
    depth_stencil_buffer = gl::make_renderbuffer();
  }

  auto colour_format = alpha ? gl::internal_format::kRgba32F : gl::internal_format::kRgb32F;
  if (samples <= 1) {
    gl::texture_storage_2d(colour_buffer, 1, dimensions, colour_format);
    gl::framebuffer_colour_texture_2d(fbo, 0, colour_buffer);
    if (depth_stencil) {
      gl::renderbuffer_storage(*depth_stencil_buffer, gl::internal_format::kDepth24Stencil8,
                               dimensions);
      gl::framebuffer_renderbuffer_depth_stencil(fbo, *depth_stencil_buffer);
    }
  } else {
    gl::texture_storage_2d_multisample(colour_buffer, samples, dimensions, colour_format);
    gl::framebuffer_colour_texture_2d_multisample(fbo, 0, colour_buffer);
    if (depth_stencil) {
      gl::renderbuffer_storage_multisample(*depth_stencil_buffer, samples,
                                           gl::internal_format::kDepth24Stencil8, dimensions);
      gl::framebuffer_renderbuffer_depth_stencil(fbo, *depth_stencil_buffer);
    }
  }
  if (!gl::is_framebuffer_complete(fbo)) {
    return unexpected("framebuffer is not complete");
  }
  return {{dimensions, samples, std::move(fbo), std::move(colour_buffer),
           std::move(depth_stencil_buffer)}};
}

struct vertex_attribute_container {
  struct type_info {
    gl::buffer buffer;
    std::size_t total_count_per_vertex = 0;
    std::size_t current_offset = 0;
  };
  gl::vertex_array vertex_array = gl::make_vertex_array();
  std::unordered_map<gl::type, type_info> buffers;

  void bind() const { gl::bind_vertex_array(vertex_array); }

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
      gl::vertex_int_attribute_buffer(info.buffer, index, count_per_vertex, gl::type_of<T>(),
                                      info.total_count_per_vertex * sizeof(T),
                                      info.current_offset * sizeof(T));
    } else {
      gl::vertex_float_attribute_buffer(info.buffer, index, count_per_vertex, gl::type_of<T>(),
                                        /* normalize */ false,
                                        info.total_count_per_vertex * sizeof(T),
                                        info.current_offset * sizeof(T));
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

  std::uint32_t iota_index_size = 0;
  std::optional<gl::buffer> iota_index_buffer;
  std::unordered_map<framebuffer, framebuffer_data> framebuffers;

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

  void refresh_framebuffers(const render::target& target) {
    auto refresh = [&](framebuffer f, const uvec2& dimensions, bool alpha, bool depth_stencil,
                       std::uint32_t samples) {
      auto it = framebuffers.find(f);
      if (it != framebuffers.end() && it->second.dimensions == dimensions &&
          it->second.samples == samples) {
        return;
      }
      auto result = make_render_framebuffer(dimensions, alpha, depth_stencil, samples);
      if (!result) {
        status = unexpected("OpenGL error: " + result.error());
        return;
      }
      framebuffers.insert_or_assign(f, std::move(*result));
    };
    auto samples = std::clamp(target.msaa(), 1u, gl::max_samples());
    refresh(framebuffer::kRender, target.screen_dimensions, /* alpha */ false,
            /* depth/stencil */ true, samples);
    refresh(framebuffer::kAux0, target.screen_dimensions, /* alpha */ true,
            /* depth/stencil */ false, 1u);
    refresh(framebuffer::kAux1, target.screen_dimensions, /* alpha */ true,
            /* depth/stencil */ false, 1u);
  }

  gl::bound_framebuffer bind_draw_framebuffer(framebuffer f) {
    if (const auto* data = get_framebuffer(f)) {
      gl::viewport(glm::uvec2{0}, data->dimensions);
      return gl::bind_draw_framebuffer(data->fbo);
    }
    return gl::bound_framebuffer{GL_FRAMEBUFFER};
  }

  gl::bound_framebuffer bind_read_framebuffer(framebuffer f) {
    if (const auto* data = get_framebuffer(f)) {
      return gl::bind_read_framebuffer(data->fbo);
    }
    return gl::bound_framebuffer{GL_FRAMEBUFFER};
  }

  const framebuffer_data* get_framebuffer(framebuffer f) {
    auto it = framebuffers.find(f);
    return it == framebuffers.end() ? nullptr : &it->second;
  }

  void oklab_resolve(const framebuffer_data& source, const fvec2& aspect_scale, bool to_srgb) {
    const auto& program = shader(shader::kOklabResolve);
    gl::use_program(program);
    gl::enable_blend(false);
    gl::enable_depth_test(false);
    gl::enable_srgb(false);
    gl::enable_clip_planes(0u);

    auto result = gl::set_uniforms(program, "screen_dimensions", source.dimensions, "aspect_scale",
                                   aspect_scale, "is_multisample", source.samples > 1,
                                   "is_output_srgb", to_srgb);
    if (!result) {
      status = unexpected("oklab_resolve shader error: " + result.error());
      return;
    }
    if (source.samples <= 1) {
      result = gl::set_uniform_texture_2d(program, "framebuffer_texture", /* texture unit */ 0,
                                          source.colour_buffer, pixel_sampler);
    } else {
      result = gl::set_uniform_texture_2d_multisample(program, "framebuffer_texture_multisample",
                                                      /* texture unit */ 0, source.colour_buffer);
    }
    if (!result) {
      status = unexpected("oklab_resolve shader error: " + result.error());
      return;
    }

    vertex_attribute_container attributes;
    attributes.add_buffer(std::span<const float>(kQuadVertexData), 2);
    attributes.bind();
    attributes.add_attribute<float>(/* position */ 0, 2);
    gl::draw_elements(gl::draw_mode::kTriangles, index_buffer(6u), gl::type_of<std::uint32_t>(), 6u,
                      0);
  }

  // TODO: use a jump-flood algorithm instead?
  void outline_mask(const framebuffer_data& source, const fvec2& aspect_scale, float pixel_radius) {
    const auto& program = shader(shader::kOutlineMask);
    gl::use_program(program);
    gl::enable_blend(false);
    gl::enable_depth_test(false);
    gl::enable_srgb(false);
    gl::enable_clip_planes(0u);

    auto result = gl::set_uniforms(program, "screen_dimensions", source.dimensions, "aspect_scale",
                                   aspect_scale, "pixel_radius", pixel_radius);
    if (!result) {
      status = unexpected("outline_mask shader error: " + result.error());
      return;
    }
    // TODO: same comment about texture unit 0 vs 1?
    result = gl::set_uniform_texture_2d(program, "framebuffer_texture", /* texture unit */ 1,
                                        source.colour_buffer, pixel_sampler);
    if (!result) {
      status = unexpected("outline_mask shader error: " + result.error());
      return;
    }

    vertex_attribute_container attributes;
    attributes.add_buffer(std::span<const float>(kQuadVertexData), 2);
    attributes.bind();
    attributes.add_attribute<float>(/* position */ 0, 2);
    gl::draw_elements(gl::draw_mode::kTriangles, index_buffer(6u), gl::type_of<std::uint32_t>(), 6u,
                      0);
  }

  void
  fullscreen_blend(const framebuffer_data& source, const fvec2& aspect_scale, float blend_alpha) {
    const auto& program = shader(shader::kFullscreenBlend);
    gl::use_program(program);
    gl::enable_blend(true);
    gl::enable_depth_test(false);
    gl::enable_srgb(false);
    gl::enable_clip_planes(0u);
    gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

    auto result =
        gl::set_uniforms(program, "aspect_scale", aspect_scale, "blend_alpha", blend_alpha);
    if (!result) {
      status = unexpected("fullscreen_blend shader error: " + result.error());
      return;
    }
    // TODO: rendering breaks if this is set to texture unit 0. No idea why! Some problem
    // with binding multisampled and non-multisampled textures to the same unit for different calls?
    result = gl::set_uniform_texture_2d(program, "framebuffer_texture", /* texture unit */ 1,
                                        source.colour_buffer, pixel_sampler);
    if (!result) {
      status = unexpected("fullscreen_blend shader error: " + result.error());
      return;
    }

    vertex_attribute_container attributes;
    attributes.add_buffer(std::span<const float>(kQuadVertexData), 2);
    attributes.bind();
    attributes.add_attribute<float>(/* position */ 0, 2);
    gl::draw_elements(gl::draw_mode::kTriangles, index_buffer(6u), gl::type_of<std::uint32_t>(), 6u,
                      0);
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

  r = load_shader(shader::kFx,
                  {{"game/render/shaders/fx/fx_fragment.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/fx/fx_geometry.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/fx/fx_vertex.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }

  r = load_shader(shader::kShapeOutline,
                  {{"game/render/shaders/shape/colour_outline.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/shape_outline.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kShapeMotion,
                  {{"game/render/shaders/shape/colour_motion.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/shape_motion.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kShapeFill,
                  {{"game/render/shaders/shape/colour_fill.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/shape/shape_fill.g.glsl", gl::shader_type::kGeometry},
                   {"game/render/shaders/shape/input.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kOklabResolve,
                  {{"game/render/shaders/post/oklab_resolve.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/post/fullscreen.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kOutlineMask,
                  {{"game/render/shaders/post/outline_mask.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/post/fullscreen.v.glsl", gl::shader_type::kVertex}});
  if (!r) {
    return unexpected(r.error());
  }
  r = load_shader(shader::kFullscreenBlend,
                  {{"game/render/shaders/post/fullscreen_blend.f.glsl", gl::shader_type::kFragment},
                   {"game/render/shaders/post/fullscreen.v.glsl", gl::shader_type::kVertex}});
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
  impl_->refresh_framebuffers(target_);
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  gl::clear_colour({0.f, 0.f, 0.f, 0.f});
  gl::clear_depth(0.);
  gl::clear(gl::clear_mask::kColourBufferBit | gl::clear_mask::kDepthBufferBit |
            gl::clear_mask::kStencilBufferBit);
}

void GlRenderer::clear_depth() const {
  impl_->refresh_framebuffers(target_);
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  gl::clear_depth(0.);
  gl::clear(gl::clear_mask::kDepthBufferBit);
}

std::int32_t GlRenderer::line_height(const font_data& font) const {
  auto font_result = impl_->font_cache.get(target(), font, ustring::ascii(""));
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return 0;
  }
  const auto& font_entry = **font_result;
  auto height = font_entry.font.line_height();
  return static_cast<std::int32_t>(std::ceil(static_cast<float>(height) / target().scale_factor()));
}

std::int32_t GlRenderer::text_width(const font_data& font, ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return 0;
  }
  const auto& font_entry = **font_result;
  auto width = font_entry.font.calculate_width(s);
  return static_cast<std::int32_t>(std::ceil(static_cast<float>(width) / target().scale_factor()));
}

ustring
GlRenderer::trim_for_width(const font_data& font, std::int32_t width, ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return ustring::ascii("");
  }
  const auto& font_entry = **font_result;
  auto screen_width =
      static_cast<std::int32_t>(std::floor(static_cast<float>(width) * target().scale_factor()));
  return font_entry.font.trim_for_width(s, screen_width);
}

void GlRenderer::render_text(const font_data& font, const fvec2& position, const cvec4& colour,
                             bool clip, ustring_view s) const {
  auto font_result = impl_->font_cache.get(target(), font, s);
  if (!font_result) {
    impl_->status = unexpected(font_result.error());
    return;
  }
  const auto& font_entry = **font_result;

  impl_->refresh_framebuffers(target_);
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  gl::texture_barrier();  // TODO: only need to do once for multiline text?
  const auto& program = impl_->shader(shader::kText);
  gl::use_program(program);
  if (clip) {
    gl::enable_clip_planes(4u);
  } else {
    gl::enable_clip_planes(0u);
  }
  gl::enable_blend(false);
  gl::enable_depth_test(false);

  const auto* render_framebuffer = impl_->get_framebuffer(framebuffer::kRender);
  if (!render_framebuffer) {
    return;
  }
  auto clip_rect = target().clip_rect();
  auto text_origin = target().render_to_iscreen_coords(position + clip_rect.min());
  auto result = gl::set_uniforms(program, "screen_dimensions", target().screen_dimensions,
                                 "is_multisample", render_framebuffer->samples > 1,
                                 "texture_dimensions", font_entry.font.bitmap_dimensions(),
                                 "clip_min", target().render_to_iscreen_coords(clip_rect.min()),
                                 "clip_max", target().render_to_iscreen_coords(clip_rect.max()),
                                 "is_font_lcd", font_entry.font.is_lcd() ? 1u : 0u, "text_colour",
                                 colour, "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected(result.error());
  }
  if (render_framebuffer->samples <= 1) {
    result = gl::set_uniform_texture_2d(program, "framebuffer_texture", /* texture unit */ 0,
                                        render_framebuffer->colour_buffer, impl_->pixel_sampler);
  } else {
    result = gl::set_uniform_texture_2d_multisample(program, "framebuffer_texture_multisample",
                                                    /* texture unit */ 0,
                                                    render_framebuffer->colour_buffer);
  }
  if (!result) {
    impl_->status = unexpected("text shader error: " + result.error());
    return;
  }
  result = gl::set_uniform_texture_2d(program, "font_texture", /* texture unit */ 1,
                                      font_entry.texture, impl_->pixel_sampler);
  if (!result) {
    impl_->status = unexpected("text shader error: " + result.error());
    return;
  }

  std::vector<std::int16_t> vertex_data;
  std::uint32_t vertices = 0;
  font_entry.font.iterate_glyph_data(
      s, text_origin,
      [&](const ivec2& position, const ivec2& texture_coords, const ivec2& dimensions) {
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
  attributes.bind();
  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* texture_coords */ 2, 2);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(vertices),
                    gl::type_of<std::uint32_t>(), vertices, 0);
}

void GlRenderer::render_text(const font_data& font, const frect& bounds, alignment align,
                             const cvec4& colour, bool clip,
                             const std::vector<ustring>& lines) const {
  auto height = static_cast<float>(line_height(font));

  auto align_x = [&](const ustring& s) -> float {
    if (+(align & render::alignment::kLeft)) {
      return 0;
    }
    if (+(align & render::alignment::kRight)) {
      return bounds.size.x - static_cast<float>(text_width(font, s));
    }
    return (bounds.size.x - static_cast<float>(text_width(font, s))) / 2.f;
  };

  auto align_y = [&]() -> float {
    if (+(align & render::alignment::kTop)) {
      return 0;
    }
    if (+(align & render::alignment::kBottom)) {
      return bounds.size.y - height * static_cast<float>(lines.size());
    }
    return (bounds.size.y - height * static_cast<float>(lines.size())) / 2;
  };

  fvec2 position{0, bounds.position.y + align_y()};
  // TODO: render all in one?
  for (const auto& s : lines) {
    position.x = bounds.position.x + align_x(s);
    render_text(font, position, colour, clip, s);
    position.y += height;
  }
}

void GlRenderer::render_panel(const panel_data& p) const {
  if (p.style == panel_style::kNone) {
    return;
  }

  impl_->refresh_framebuffers(target_);
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  const auto& program = impl_->shader(shader::kPanel);
  gl::use_program(program);
  gl::enable_clip_planes(4u);
  gl::enable_blend(true);
  gl::enable_depth_test(false);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);

  auto clip_rect = target().clip_rect();
  auto result = gl::set_uniforms(program, "screen_dimensions", target().screen_dimensions,
                                 "clip_min", target().render_to_iscreen_coords(clip_rect.min()),
                                 "clip_max", target().render_to_iscreen_coords(clip_rect.max()),
                                 "colour_cycle", colour_cycle_ / 256.f);
  if (!result) {
    impl_->status = unexpected("panel shader error: " + result.error());
    return;
  }

  auto min = target().render_to_iscreen_coords(clip_rect.min() + p.bounds.min());
  auto size = target().render_to_iscreen_coords(clip_rect.min() + p.bounds.max()) - min;

  auto border_size = static_cast<std::int16_t>(target().border_size(1));
  std::vector<float> float_data = {p.colour.r, p.colour.g, p.colour.b, p.colour.a,
                                   p.border.r, p.border.g, p.border.b, p.border.a};
  std::vector<std::int16_t> int_data = {
      static_cast<std::int16_t>(min.x), static_cast<std::int16_t>(min.y),
      static_cast<std::int16_t>(size.x), static_cast<std::int16_t>(size.y),
      // TODO: render dimensions shouldn't really be integer any more!
      static_cast<std::int16_t>(p.bounds.size.x), static_cast<std::int16_t>(p.bounds.size.y),
      static_cast<std::int16_t>(p.style), border_size};

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const float>{float_data}, 8);
  attributes.add_buffer(std::span<const std::int16_t>{int_data}, 8);
  attributes.bind();
  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* screen_dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* render_dimensions */ 2, 2);
  attributes.add_attribute<float>(/* colour */ 3, 4);
  attributes.add_attribute<float>(/* border */ 4, 4);
  attributes.add_attribute<std::int16_t>(/* style */ 5, 1);
  attributes.add_attribute<std::int16_t>(/* border_size */ 6, 1);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(1), gl::type_of<unsigned>(), 1, 0);
}

void GlRenderer::render_panel(const combo_panel& data) const {
  auto panel_copy = data.panel;
  panel_copy.bounds.position =
      glm::min(target().clip_rect().size - panel_copy.bounds.size, panel_copy.bounds.position);
  panel_copy.bounds.position = glm::max(fvec2{0}, panel_copy.bounds.position);
  panel_copy.bounds.position = target().snap_render_to_screen_coords(panel_copy.bounds.position);
  if (panel_copy.bounds.size.x == 0.f) {
    // Auto-size x.
    float max_width = 0.f;
    for (const auto& e : data.elements) {
      if (e.bounds.size.x != 0.f) {
        max_width = std::max(max_width, 2.f * data.padding.x + e.bounds.max().x);
      } else if (const auto* text = std::get_if<combo_panel::text>(&e.e);
                 text && !text->multiline) {
        max_width = std::max(
            max_width,
            2.f * data.padding.x + e.bounds.min().x + text_width(text->font, text->content));
      }
    }
    panel_copy.bounds.position.x -= max_width / 2.f;
    panel_copy.bounds.size.x = max_width;
  }
  render_panel(panel_copy);

  auto border_size = target().border_size(1);
  auto inner_padding = fvec2{data.padding - border_size};
  auto& t = const_cast<render::target&>(target_);
  render::clip_handle clip{t, panel_copy.bounds.contract(glm::ivec2{border_size})};
  for (const auto& e : data.elements) {
    if (const auto* icon = std::get_if<combo_panel::icon>(&e.e)) {
      render::clip_handle icon_clip{t, e.bounds + inner_padding};
      auto shapes = icon->shapes;
      std::vector<fx> no_fx;
      render_shapes(coordinate_system::kCentered, shapes, no_fx, shape_style::kIcon);
    } else if (const auto* text = std::get_if<combo_panel::text>(&e.e)) {
      auto bounds = e.bounds;
      if (bounds.size.x == 0.f) {
        bounds.size.x = panel_copy.bounds.size.x - 2.f * data.padding.x;
      }
      auto lines = prepare_text(*this, text->font, text->multiline,
                                static_cast<std::int32_t>(bounds.size.x), text->content);
      if (text->drop_shadow) {
        render_text(text->font, bounds + inner_padding + fvec2{text->drop_shadow->offset},
                    text->align, cvec4{0.f, 0.f, 0.f, text->drop_shadow->alpha},
                    /* clip */ true, lines);
      }
      render_text(text->font, bounds + inner_padding, text->align, text->colour,
                  /* clip */ true, lines);
    }
  }
}

void GlRenderer::render_background(const render::background& data) const {
  if (data.data0.type == render::background::type::kNone &&
      data.data1.type == render::background::type::kNone) {
    return;
  }

  impl_->refresh_framebuffers(target_);
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  const auto& program = impl_->shader(shader::kBackground);
  gl::use_program(program);
  gl::enable_clip_planes(4u);
  gl::enable_blend(false);
  gl::enable_depth_test(false);

  auto clip_rect = target().clip_rect();
  auto result = gl::set_uniforms(
      program, "is_multisample", target().msaa() > 1u, "screen_dimensions",
      target().screen_dimensions, "clip_min", target().render_to_iscreen_coords(clip_rect.min()),
      "clip_max", target().render_to_iscreen_coords(clip_rect.max()), "position", data.position,
      "rotation", data.rotation, "interpolate", std::clamp(data.interpolate, 0.f, 1.f), "type0",
      static_cast<std::uint32_t>(data.data0.type), "type1",
      static_cast<std::uint32_t>(data.data1.type), "colour0", data.data0.colour, "colour1",
      data.data1.colour, "parameters0", data.data0.parameters, "parameters1",
      data.data1.parameters);
  if (!result) {
    impl_->status = unexpected("background shader error: " + result.error());
    return;
  }

  auto min = target().render_to_iscreen_coords(clip_rect.min());
  auto size = target().render_to_iscreen_coords(clip_rect.max()) - min;

  std::vector<std::int16_t> int_data = {static_cast<std::int16_t>(min.x),
                                        static_cast<std::int16_t>(min.y),
                                        static_cast<std::int16_t>(size.x),
                                        static_cast<std::int16_t>(size.y),
                                        static_cast<std::int16_t>(clip_rect.size.x),
                                        static_cast<std::int16_t>(clip_rect.size.y)};

  vertex_attribute_container attributes;
  attributes.add_buffer(std::span<const std::int16_t>{int_data}, 7);
  attributes.bind();
  attributes.add_attribute<std::int16_t>(/* position */ 0, 2);
  attributes.add_attribute<std::int16_t>(/* screen_dimensions */ 1, 2);
  attributes.add_attribute<std::int16_t>(/* render_dimensions */ 2, 2);
  gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(1), gl::type_of<unsigned>(), 1, 0);
}

void GlRenderer::render_shapes(coordinate_system ctype, std::vector<shape>& shapes,
                               std::vector<fx>& fxs, shape_style style) const {
  std::stable_sort(shapes.begin(), shapes.end(),
                   [](const shape& a, const shape& b) { return a.z < b.z; });
  std::stable_sort(fxs.begin(), fxs.end(), [](const fx& a, const fx& b) { return a.z < b.z; });

  struct shape_data {
    std::uint32_t buffer_index = 0;
    std::uint32_t style = 0;
    uvec2 params{0u, 0u};
    float rotation = 0.f;
    float line_width = 0.f;
    float z = 0.f;
    fvec2 position{0.f};
    fvec2 dimensions{0.f};
    cvec4 colour0{0.f};
    cvec4 colour1{0.f};
  };

  struct shape_buffer_data {
    cvec4 colour0{0.f};
    cvec4 colour1{0.f};
    std::uint32_t style = 0;
    std::uint32_t ball_index = 0;
    uvec2 padding{0};
  };

  struct ball_buffer_data {
    fvec2 position{0.f};
    fvec2 dimensions{0.f};
    float line_width = 0.f;
    float padding = 0.f;
  };

  static thread_local std::vector<float> vertex_float_data;
  static thread_local std::vector<std::uint32_t> vertex_int_data;
  static thread_local std::vector<shape_buffer_data> buffer_data;
  static thread_local std::vector<ball_buffer_data> ball_data;

  static thread_local std::vector<unsigned> shadow_trail_indices;
  static thread_local std::vector<unsigned> shadow_fill_indices;
  static thread_local std::vector<unsigned> shadow_outline_indices;
  static thread_local std::vector<unsigned> bottom_outline_indices;
  static thread_local std::vector<unsigned> trail_indices;
  static thread_local std::vector<unsigned> fill_indices;
  static thread_local std::vector<unsigned> outline_indices;

  vertex_float_data.clear();
  vertex_int_data.clear();
  buffer_data.clear();
  ball_data.clear();
  shadow_trail_indices.clear();
  shadow_fill_indices.clear();
  shadow_outline_indices.clear();
  bottom_outline_indices.clear();
  trail_indices.clear();
  fill_indices.clear();
  outline_indices.clear();

  auto add_shape_data = [&](const shape_data& d) {
    vertex_int_data.emplace_back(static_cast<std::uint32_t>(buffer_data.size()));
    vertex_int_data.emplace_back(d.style);
    vertex_int_data.emplace_back(d.params.x);
    vertex_int_data.emplace_back(d.params.y);
    vertex_float_data.emplace_back(d.rotation);
    vertex_float_data.emplace_back(d.line_width);
    vertex_float_data.emplace_back(d.position.x);
    vertex_float_data.emplace_back(d.position.y);
    vertex_float_data.emplace_back(d.z);
    vertex_float_data.emplace_back(d.dimensions.x);
    vertex_float_data.emplace_back(d.dimensions.y);

    std::uint32_t ball_index = 0;
    if (d.style == kStyleBall) {
      ball_index = ball_data.size();
      ball_data.emplace_back(ball_buffer_data{d.position, d.dimensions, d.line_width});
    }
    buffer_data.emplace_back(shape_buffer_data{
        colour::hsl2oklab_cycle(d.colour0, colour_cycle_ / 256.f),
        colour::hsl2oklab_cycle(d.colour1, colour_cycle_ / 256.f), d.style, ball_index});
  };

  std::optional<fvec2> shadow_offset;
  if (style == shape_style::kStandard) {
    shadow_offset = {4, 6};
  } else if (style == shape_style::kIcon) {
    shadow_offset = {3, 3};
  }

  std::uint32_t vertex_index = 0;
  auto add_outline_data = [&](const shape_data& d,
                              const std::optional<render::motion_trail>& trail) {
    add_shape_data(d);
    if (d.z < colour::kZTrails) {
      bottom_outline_indices.emplace_back(vertex_index++);
    } else {
      outline_indices.emplace_back(vertex_index++);
    }
    if (trail) {
      auto dt = d;
      dt.position = trail->prev_origin;
      dt.rotation = trail->prev_rotation;
      dt.colour0 = trail->prev_colour0;
      dt.colour1 = trail->prev_colour1.value_or(trail->prev_colour0);
      add_shape_data(dt);
      trail_indices.emplace_back(vertex_index - 1);
      trail_indices.emplace_back(vertex_index++);
    }
    if (shadow_offset) {
      auto ds = d;
      ds.position += *shadow_offset;
      ds.colour0 = cvec4{0.f, 0.f, 0.f, ds.colour0.a * colour::kShadowAlpha0};
      ds.colour1 = cvec4{0.f, 0.f, 0.f, ds.colour1.a * colour::kShadowAlpha0};
      ds.line_width += 1.f;
      add_shape_data(ds);
      shadow_outline_indices.emplace_back(vertex_index++);
      if (trail) {
        auto dt = ds;
        dt.position = trail->prev_origin + *shadow_offset;
        dt.rotation = trail->prev_rotation;
        dt.colour0 = {0.f, 0.f, 0.f, trail->prev_colour0.a * colour::kShadowAlpha0};
        dt.colour1 = {0.f, 0.f, 0.f,
                      trail->prev_colour1.value_or(trail->prev_colour0).a * colour::kShadowAlpha0};
        add_shape_data(dt);
        shadow_trail_indices.emplace_back(vertex_index - 1);
        shadow_trail_indices.emplace_back(vertex_index++);
      }
    }
  };

  auto add_fill_data = [&](const shape_data& d) {
    add_shape_data(d);
    fill_indices.emplace_back(vertex_index++);
    if (style == shape_style::kStandard) {
      auto ds = d;
      ds.position += *shadow_offset;
      ds.colour0 = {0.f, 0.f, 0.f, ds.colour0.a / 2.f};
      ds.colour1 = {0.f, 0.f, 0.f, ds.colour1.a / 2.f};
      add_shape_data(ds);
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
    switch (shape.data.index()) {
      VARIANT_CASE_GET(render::ngon, shape.data, p) {
        auto add_polygon = [&](std::uint32_t style, std::uint32_t param) {
          add_outline_data(
              {
                  .style = style,
                  .params = {p.sides, param},
                  .rotation = shape.rotation,
                  .line_width = p.line_width,
                  .z = shape.z,
                  .position = shape.origin,
                  .dimensions = {p.radius, p.inner_radius},
                  .colour0 = shape.colour0,
                  .colour1 = shape.colour1.value_or(shape.colour0),
              },
              shape.trail);
        };
        if (p.style != ngon_style::kPolygram || p.sides <= 3) {
          add_polygon(p.style == ngon_style::kPolystar ? kStyleNgonPolystar : kStyleNgonPolygon,
                      p.segments);
        } else if (p.sides == 4) {
          add_polygon(kStyleNgonPolystar, p.segments);
          add_polygon(kStyleNgonPolygon, p.segments);
        } else {
          for (std::uint32_t i = 0; i + 2 < p.sides; ++i) {
            add_polygon(kStyleNgonPolygram, i);
          }
          add_polygon(kStyleNgonPolygon, p.segments);
        }
        break;
      }

      VARIANT_CASE_GET(render::box, shape.data, p) {
        add_outline_data(
            {
                .style = kStyleBox,
                .rotation = shape.rotation,
                .line_width = p.line_width,
                .z = shape.z,
                .position = shape.origin,
                .dimensions = p.dimensions,
                .colour0 = shape.colour0,
                .colour1 = shape.colour1.value_or(shape.colour0),
            },
            shape.trail);
        break;
      }

      VARIANT_CASE_GET(render::ball, shape.data, p) {
        add_outline_data(
            {
                .style = kStyleBall,
                .rotation = shape.rotation,
                .line_width = p.line_width,
                .z = shape.z,
                .position = shape.origin,
                .dimensions = {p.radius, p.inner_radius},
                .colour0 = shape.colour0,
                .colour1 = shape.colour1.value_or(shape.colour0),
            },
            shape.trail);
        break;
      }

      VARIANT_CASE_GET(render::line, shape.data, p) {
        add_outline_data(
            {
                .style = kStyleLine,
                .params = {p.sides, 0},
                .rotation = shape.rotation,
                .line_width = p.line_width,
                .z = shape.z,
                .position = shape.origin,
                .dimensions = {p.radius, 0},
                .colour0 = shape.colour0,
                .colour1 = shape.colour1.value_or(shape.colour0),
            },
            shape.trail);
        break;
      }

      VARIANT_CASE_GET(render::ngon_fill, shape.data, p) {
        add_fill_data({
            .style = kStyleNgonPolygon,
            .params = {p.sides, p.segments},
            .rotation = shape.rotation,
            .z = shape.z,
            .position = shape.origin,
            .dimensions = {p.radius, p.inner_radius},
            .colour0 = shape.colour0,
            .colour1 = shape.colour1.value_or(shape.colour0),
        });
        break;
      }

      VARIANT_CASE_GET(render::box_fill, shape.data, p) {
        add_fill_data({
            .style = kStyleBox,
            .rotation = shape.rotation,
            .z = shape.z,
            .position = shape.origin,
            .dimensions = p.dimensions,
            .colour0 = shape.colour0,
            .colour1 = shape.colour1.value_or(shape.colour0),
        });
        break;
      }

      VARIANT_CASE_GET(render::ball_fill, shape.data, p) {
        add_fill_data({
            .style = kStyleBall,
            .rotation = shape.rotation,
            .z = shape.z,
            .position = shape.origin,
            .dimensions = {p.radius, p.inner_radius},
            .colour0 = shape.colour0,
            .colour1 = shape.colour1.value_or(shape.colour0),
        });
        break;
      }
    }
  }

  // TODO: should all the buffers be saved between frames?
  auto shape_buffer = make_stream_draw_buffer(std::span<const shape_buffer_data>{buffer_data});
  auto ball_buffer = make_stream_draw_buffer(std::span<const ball_buffer_data>{ball_data});
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

  vertex_attribute_container shape_attributes;
  shape_attributes.add_buffer(std::span<const std::uint32_t>(vertex_int_data), 4);
  shape_attributes.add_buffer(std::span<const float>{vertex_float_data}, 7);

  shape_attributes.bind();
  shape_attributes.add_attribute<std::uint32_t>(/* buffer_index */ 0, 1);
  shape_attributes.add_attribute<std::uint32_t>(/* style */ 1, 1);
  shape_attributes.add_attribute<std::uint32_t>(/* params */ 2, 2);
  shape_attributes.add_attribute<float>(/* rotation */ 3, 1);
  shape_attributes.add_attribute<float>(/* line_width */ 4, 1);
  shape_attributes.add_attribute<float>(/* position */ 5, 3);
  shape_attributes.add_attribute<float>(/* dimensions */ 6, 2);

  vertex_attribute_container fx_attributes;
  std::uint32_t fx_count = 0;
  if (!fxs.empty()) {
    vertex_int_data.clear();
    vertex_float_data.clear();

    struct fx_data {
      fx_shape shape = fx_shape::kFxShapeBall;
      fx_style style = fx_style::kNone;
      float time = 0.f;
      float rotation = 0.f;
      fvec4 colour{0.f};
      fvec2 position{0.f};
      fvec2 dimensions{0.f};
      fvec2 seed{0.f};
    };

    auto add_fx_data = [&](const fx_data& d) {
      if (d.style == fx_style::kNone) {
        return;
      }
      vertex_int_data.emplace_back(static_cast<std::uint32_t>(d.shape));
      vertex_int_data.emplace_back(static_cast<std::uint32_t>(d.style));
      vertex_float_data.emplace_back(d.time);
      vertex_float_data.emplace_back(d.rotation);
      vertex_float_data.emplace_back(d.colour.r);
      vertex_float_data.emplace_back(d.colour.g);
      vertex_float_data.emplace_back(d.colour.b);
      vertex_float_data.emplace_back(d.colour.a);
      vertex_float_data.emplace_back(d.position.x);
      vertex_float_data.emplace_back(d.position.y);
      vertex_float_data.emplace_back(d.dimensions.x);
      vertex_float_data.emplace_back(d.dimensions.y);
      vertex_float_data.emplace_back(d.seed.x);
      vertex_float_data.emplace_back(d.seed.y);
      ++fx_count;
    };

    for (const auto& d : fxs) {
      switch (d.data.index()) {
        VARIANT_CASE_GET(ball_fx, d.data, v) {
          add_fx_data({
              .shape = kFxShapeBall,
              .style = d.style,
              .time = d.time,
              .rotation = 0.f,
              .colour = d.colour,
              .position = v.position,
              .dimensions = {v.radius, v.inner_radius},
              .seed = d.seed,
          });
          break;
        }

        VARIANT_CASE_GET(box_fx, d.data, v) {
          add_fx_data({
              .shape = kFxShapeBox,
              .style = d.style,
              .time = d.time,
              .rotation = v.rotation,
              .colour = d.colour,
              .position = v.position,
              .dimensions = v.dimensions,
              .seed = d.seed,
          });
          break;
        }
      }
    }
    fx_attributes.add_buffer(std::span<const std::uint32_t>(vertex_int_data), 2);
    fx_attributes.add_buffer(std::span<const float>{vertex_float_data}, 12);

    fx_attributes.bind();
    fx_attributes.add_attribute<float>(/* time */ 0, 1);
    fx_attributes.add_attribute<float>(/* rotation */ 1, 1);
    fx_attributes.add_attribute<std::uint32_t>(/* shape */ 2, 1);
    fx_attributes.add_attribute<std::uint32_t>(/* style */ 3, 1);
    fx_attributes.add_attribute<float>(/* colour */ 4, 4);
    fx_attributes.add_attribute<float>(/* position */ 5, 2);
    fx_attributes.add_attribute<float>(/* dimensions */ 6, 2);
    fx_attributes.add_attribute<float>(/* seed */ 7, 2);
  }

  auto clip_rect = target().clip_rect();
  auto top_rect = target().top_clip_rect();
  fvec2 offset;
  switch (ctype) {
  case coordinate_system::kGlobal:
    offset = fvec2{0, 0};
    break;
  case coordinate_system::kLocal:
    offset = top_rect.min();
    break;
  case coordinate_system::kCentered:
    offset = (top_rect.min() + top_rect.max()) / 2.f;
  }

  auto clip_min = target().snap_render_to_screen_coords(clip_rect.min());
  auto clip_max = target().snap_render_to_screen_coords(clip_rect.max());
  auto set_uniforms = [&](const gl::program& program, bool needs_multisample) {
    if (needs_multisample) {
      auto r = gl::set_uniforms(program, "is_multisample", target().msaa() > 1u);
      if (!r) {
        return r;
      }
    }
    return gl::set_uniforms(program, "aspect_scale", target().aspect_scale(), "render_dimensions",
                            target().render_dimensions, "screen_dimensions",
                            target().screen_dimensions, "clip_min", clip_min, "clip_max", clip_max,
                            "coordinate_offset", offset);
  };

  auto render_pass = [&](shader s, gl::draw_mode mode, const gl::buffer& index_buffer,
                         std::size_t count) {
    const auto& program = impl_->shader(s);
    gl::use_program(program);
    bool needs_multisample = s == shader::kShapeFill || s == shader::kShapeOutline;
    if (auto result = set_uniforms(program, needs_multisample); !result) {
      impl_->status = unexpected("shader " + std::to_string(static_cast<std::uint32_t>(s)) +
                                 " error: " + result.error());
    } else {
      gl::draw_elements(mode, index_buffer, gl::type_of<unsigned>(), count, 0);
    }
  };

  impl_->refresh_framebuffers(target_);
  gl::clear_depth(0.f);
  gl::enable_clip_planes(4u);
  gl::enable_depth_test(false);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  gl::bind_shader_storage_buffer(shape_buffer, 0);
  gl::bind_shader_storage_buffer(ball_buffer, 1);

  // Shadow pass.
  {
    auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
    shape_attributes.bind();
    if (!shadow_trail_indices.empty()) {
      render_pass(shader::kShapeMotion, gl::draw_mode::kLines, shadow_trail_index_buffer,
                  shadow_trail_indices.size());
    }
    if (!shadow_outline_indices.empty()) {
      render_pass(shader::kShapeOutline, gl::draw_mode::kPoints, shadow_outline_index_buffer,
                  shadow_outline_indices.size());
    }
    if (!shadow_fill_indices.empty()) {
      render_pass(shader::kShapeFill, gl::draw_mode::kPoints, shadow_fill_index_buffer,
                  shadow_fill_indices.size());
    }
  }

  // FX pass.
  const auto* aux0 = impl_->get_framebuffer(framebuffer::kAux0);
  const auto* aux1 = impl_->get_framebuffer(framebuffer::kAux1);
  if (!fxs.empty() && aux0 && aux1) {
    gl::framebuffer_detach_colour_texture(aux1->fbo, 0u);
    gl::framebuffer_colour_texture_2d(aux0->fbo, 1u, aux1->colour_buffer);
    gl::enable_clip_planes(4u);
    gl::enable_depth_test(false);
    gl::enable_blend(true);
    gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
    {
      auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kAux0);
      gl::draw_buffers(2u);
      gl::clear_colour(colour::alpha(colour::hsl2oklab(colour::kOutline), 0.f));
      gl::clear(gl::clear_mask::kColourBufferBit | gl::clear_mask::kDepthBufferBit);

      const auto& program = impl_->shader(shader::kFx);
      gl::use_program(program);
      fx_attributes.bind();
      if (auto result = set_uniforms(program, true); !result) {
        impl_->status =
            unexpected("shader " + std::to_string(static_cast<std::uint32_t>(shader::kFx)) +
                       " error: " + result.error());
      } else {
        gl::draw_elements(gl::draw_mode::kPoints, impl_->index_buffer(fx_count),
                          gl::type_of<unsigned>(), fx_count, 0);
      }
      gl::draw_buffers(1u);
    }

    gl::framebuffer_detach_colour_texture(aux0->fbo, 1u);
    gl::framebuffer_colour_texture_2d(aux1->fbo, 0u, aux1->colour_buffer);
    gl::colour_mask({false, false, false, true});
    {
      static constexpr float kOutlineWidth = 1.5f;
      auto outline_pixels = kOutlineWidth * target().scale_factor();
      auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kAux0);
      impl_->outline_mask(*aux1, target().aspect_scale(), outline_pixels);
    }
    gl::colour_mask(glm::bvec4{true});

    auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
    impl_->fullscreen_blend(*aux0, target().aspect_scale(), colour::kEffectAlpha0);
  }

  // Shape pass.
  auto fbo_bind = impl_->bind_draw_framebuffer(framebuffer::kRender);
  shape_attributes.bind();
  gl::enable_clip_planes(4u);
  gl::enable_depth_test(true);
  gl::depth_function(gl::comparison::kGreaterEqual);
  gl::enable_blend(true);
  gl::blend_function(gl::blend_factor::kSrcAlpha, gl::blend_factor::kOneMinusSrcAlpha);
  gl::clear(gl::clear_mask::kDepthBufferBit);

  if (!bottom_outline_indices.empty()) {
    render_pass(shader::kShapeOutline, gl::draw_mode::kPoints, bottom_outline_index_buffer,
                bottom_outline_indices.size());
  }
  if (!trail_indices.empty()) {
    render_pass(shader::kShapeMotion, gl::draw_mode::kLines, trail_index_buffer,
                trail_indices.size());
  }
  gl::enable_depth_test(false);
  if (!fill_indices.empty()) {
    render_pass(shader::kShapeFill, gl::draw_mode::kPoints, fill_index_buffer, fill_indices.size());
  }
  if (!outline_indices.empty()) {
    render_pass(shader::kShapeOutline, gl::draw_mode::kPoints, outline_index_buffer,
                outline_indices.size());
  }
}

void GlRenderer::render_present(const glm::uvec2& dimensions) const {
  gl::viewport(glm::uvec2{0}, dimensions);
  if (const auto* render_framebuffer = impl_->get_framebuffer(framebuffer::kRender)) {
    impl_->oklab_resolve(*render_framebuffer, fvec2{1.f}, /* to sRGB */ true);
  }
}

}  // namespace ii::render
