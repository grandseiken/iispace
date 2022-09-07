#include "game/core/ui/toolkit.h"
#include "game/common/ustring_convert.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

void Panel::update_content(const input_frame&) {
  auto r = bounds().size_rect().contract(margin_).contract(padding_);
  for (auto& e : *this) {
    e->set_bounds(r);
  }
}

void Panel::render_content(render::GlRenderer& r) const {
  r.render_panel({
      .style = style_,
      .colour = colour_,
      .bounds = bounds().size_rect().contract(margin_),
  });
}

void TextElement::render_content(render::GlRenderer& r) const {
  if (!multiline_) {
    auto text = r.trim_for_width(font_, font_dimensions_, bounds().size.x, text_);
    r.render_text(font_, font_dimensions_, glm::ivec2{0}, colour_, text);
    return;
  }

  // TODO: quite inefficient.
  std::u32string u32;
  iterate_as_utf32(text_, [&](std::size_t, std::uint32_t c) { u32 += static_cast<char32_t>(c); });

  auto text_width = [&](std::size_t start, std::size_t end) {
    return r.text_width(font_, font_dimensions_,
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
      if (text_width(line_start, line_end) > bounds().size.x) {
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

  auto line_height = r.line_height(font_, font_dimensions_);
  glm::ivec2 position{0};
  for (const auto& s : lines) {
    // TODO: render all in one?
    r.render_text(font_, font_dimensions_, position, colour_, ustring_view::utf32(s));
    position.y += line_height;
  }
}

}  // namespace ii::ui