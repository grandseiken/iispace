#include "game/core/ui/toolkit.h"
#include "game/common/ustring_convert.h"
#include "game/render/gl_renderer.h"
#include <unordered_set>

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

void LinearLayout::update_content(const input_frame&) {
  std::unordered_set<const Element*> children_set;
  for (const auto& e : *this) {
    children_set.emplace(e.get());
  }
  for (auto it = info_.begin(); it != info_.end();) {
    if (children_set.contains(it->first)) {
      ++it;
    } else {
      it = info_.erase(it);
    }
  }

  auto get_info = [&](const Element* c) {
    auto it = info_.find(c);
    return it == info_.end() ? element_info{} : it->second;
  };

  float total_weight = 0.f;
  std::int32_t total_absolute = 0;
  for (const auto& e : *this) {
    auto info = get_info(e.get());
    if (info.absolute) {
      total_absolute += *info.absolute;
    } else {
      total_weight += info.relative_weight.value_or(1.f);
    }
  }

  auto total_space = type_ == orientation::kVertical ? bounds().size.y : bounds().size.x;
  auto free_space = std::max(0, total_space - total_absolute) -
      std::max(0, static_cast<std::int32_t>(size()) - 1) * spacing_;
  auto relative_width = [&](const element_info& info) {
    return static_cast<std::int32_t>(std::round(static_cast<float>(free_space) *
                                                info.relative_weight.value_or(1.f) / total_weight));
  };

  std::int32_t allocated_space = 0;
  std::int32_t weighted_count = 0;
  for (const auto& e : *this) {
    auto info = get_info(e.get());
    if (!info.absolute) {
      allocated_space += relative_width(info);
      ++weighted_count;
    }
  }
  auto error_space = free_space - allocated_space;

  std::int32_t position = 0;
  std::size_t weighted_i = 0;
  for (const auto& e : *this) {
    auto info = get_info(e.get());
    std::int32_t element_space = 0;
    if (info.absolute) {
      element_space = std::min(*info.absolute, total_space - position);
    } else if (free_space < 0) {
      continue;
    } else {
      element_space = relative_width(info);
      bool extra = std::abs(error_space) >
          (static_cast<std::int32_t>(weighted_i) * std::abs(error_space)) % weighted_count;
      if (extra) {
        element_space += error_space > 0 ? 1 : -1;
      }
      ++weighted_i;
    }

    if (type_ == orientation::kVertical) {
      e->set_bounds({{0, position}, {bounds().size.x, element_space}});
    } else {
      e->set_bounds({{position, 0}, {element_space, bounds().size.y}});
    }
    position += element_space + spacing_;
  }
}

void GridLayout::update_content(const input_frame&) {
  auto free_space =
      bounds().size.x - std::max(0, static_cast<std::int32_t>(columns_) - 1) * spacing_;
  auto grid_size = free_space / columns_;
  auto error_space = free_space - grid_size * columns_;

  std::int32_t i = 0;
  std::int32_t x_position = 0;
  for (const auto& e : *this) {
    auto x = i % columns_;
    auto y = i / columns_;
    ++i;
    if (!x) {
      x_position = 0;
    }

    bool extra = std::abs(error_space) > (x * std::abs(error_space)) % columns_;
    auto width = grid_size + (extra > 0 ? 1 : -1);

    e->set_bounds({{x_position, y * (grid_size + spacing_)}, {width, grid_size}});
    x_position += width + spacing_;
  }
}

}  // namespace ii::ui