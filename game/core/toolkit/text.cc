#include "game/core/toolkit/text.h"
#include "game/common/ustring_convert.h"
#include "game/render/gl_renderer.h"
#include <cstddef>

namespace ii::ui {
namespace {

std::vector<std::u32string> split_lines(ustring_view s, render::GlRenderer& r,
                                        const render::font_data& font, std::int32_t bounds_width) {
  auto u32 = to_utf32(s);

  auto text_width = [&](std::size_t start, std::size_t end) {
    return r.text_width(font,
                        ustring_view::utf32(std::u32string_view{u32}.substr(start, end - start)));
  };
  auto is_whitespace = [&](std::size_t i) {
    return u32[i] == ' ' || u32[i] == '\t' || u32[i] == '\n' || u32[i] == '\r';
  };
  auto can_break = [&](std::size_t end) { return end == u32.size() || is_whitespace(end); };
  auto get_line = [&](std::size_t start, std::size_t end) {
    while (start < end && is_whitespace(start)) {
      ++start;
    }
    while (end > start && is_whitespace(end - 1)) {
      --end;
    }
    return u32.substr(start, end - start);
  };

  std::vector<std::u32string> lines;
  std::size_t line_start = 0;
  while (line_start != u32.size()) {
    std::size_t line_end = line_start;
    while (true) {
      if (text_width(line_start, line_end) > bounds_width) {
        if (line_end > line_start) {
          --line_end;
        }
        break;
      }
      ++line_end;
      if (line_end == u32.size()) {
        break;
      }
    }

    bool done = false;
    for (auto i = line_start; i < line_end; ++i) {
      if (u32[i] == '\n') {
        lines.emplace_back(get_line(line_start, i));
        line_start = i + 1;
        done = true;
        break;
      }
    }
    if (done) {
      continue;
    }

    if (line_end == u32.size()) {
      lines.emplace_back(get_line(line_start, line_end));
      break;
    }

    for (auto i = line_end; i > line_start; --i) {
      if (can_break(i)) {
        lines.emplace_back(get_line(line_start, i));
        line_start = i + 1;
        while (line_start < u32.size() && is_whitespace(line_start)) {
          ++line_start;
        }
        done = true;
        break;
      }
    }

    if (done) {
      continue;
    }
    lines.emplace_back(get_line(line_start, line_end));
    line_start = line_end;
  }
  return lines;
}

}  // namespace

void TextElement::render_content(render::GlRenderer& r) const {
  if (cached_bounds_ != bounds()) {
    cached_bounds_ = bounds();
    dirty_ = true;
  }
  if (render_dimensions_ != r.target().render_dimensions) {
    render_dimensions_ = r.target().render_dimensions;
    dirty_ = true;
  }

  if (dirty_) {
    if (!multiline_) {
      lines_.clear();
      lines_.emplace_back(to_utf32(r.trim_for_width(font_, bounds().size.x, text_)));
    } else {
      lines_ = split_lines(text_, r, font_, bounds().size.x);
    }
    dirty_ = false;
  }
  auto line_height = r.line_height(font_);

  auto align_x = [&](std::u32string s) -> std::int32_t {
    if (+(align_ & render::alignment::kLeft)) {
      return 0;
    }
    if (+(align_ & render::alignment::kRight)) {
      return bounds().size.x - r.text_width(font_, ustring_view::utf32(s));
    }
    return (bounds().size.x - r.text_width(font_, ustring_view::utf32(s))) / 2;
  };

  auto align_y = [&]() -> std::int32_t {
    if (+(align_ & render::alignment::kTop)) {
      return 0;
    }
    if (+(align_ & render::alignment::kBottom)) {
      return bounds().size.y - line_height * static_cast<std::int32_t>(lines_.size());
    }
    return (bounds().size.y - line_height * static_cast<std::int32_t>(lines_.size())) / 2;
  };

  glm::ivec2 position{0, align_y()};
  // TODO: render all in one?
  if (drop_shadow_) {
    for (const auto& s : lines_) {
      position.x = align_x(s);
      r.render_text(font_, position + drop_shadow_->offset,
                    glm::vec4{0.f, 0.f, 0.f, drop_shadow_->opacity}, /* clip */ false,
                    ustring_view::utf32(s));
      position.y += line_height;
    }
  }
  position = glm::ivec2{0, align_y()};
  for (const auto& s : lines_) {
    position.x = align_x(s);
    r.render_text(font_, position, colour_, /* clip */ false, ustring_view::utf32(s));
    position.y += line_height;
  }
}

}  // namespace ii::ui