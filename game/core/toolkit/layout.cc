#include "game/core/toolkit/layout.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"
#include <cstddef>
#include <unordered_set>

namespace ii::ui {

void LinearLayout::update_content(const input_frame&, ui::output_frame&) {
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

  std::int32_t position = align_end_ && !weighted_count ? free_space : 0;
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

bool LinearLayout::handle_focus(const input_frame& input, ui::output_frame& output) {
  std::int32_t offset = 0;
  if ((input.pressed(ui::key::kDown) && type_ == orientation::kVertical) ||
      (input.pressed(ui::key::kRight) && type_ == orientation::kHorizontal)) {
    ++offset;
  }
  if ((input.pressed(ui::key::kUp) && type_ == orientation::kVertical) ||
      (input.pressed(ui::key::kLeft) && type_ == orientation::kHorizontal)) {
    --offset;
  }
  if (!offset || empty()) {
    return false;
  }
  std::optional<std::size_t> focus_index;
  for (std::size_t i = 0; i < size(); ++i) {
    if ((*this)[i] == focused_child()) {
      focus_index = i;
    }
  }
  std::size_t u_offset = offset > 0 ? 1u : size() - 1u;
  std::size_t index = 0;
  auto is_end = [&](std::size_t i) {
    return !wrap_focus_ && ((!i && offset < 0u) || (i == size() - 1u && offset > 0u));
  };
  if (focus_index) {
    if (is_end(*focus_index)) {
      return false;
    }
    index = (*focus_index + u_offset) % size();
  } else {
    focus_index = offset > 0u ? size() - 1u : 0u;
    index = offset > 0u ? 0u : size() - 1u;
  }

  for (; index != focus_index; index = (index + u_offset) % size()) {
    if ((*this)[index]->focus()) {
      output.sounds.emplace(sound::kMenuClick);
      return true;
    }
    if (is_end(index)) {
      break;
    }
  }
  return false;
}

void GridLayout::update_content(const input_frame&, ui::output_frame&) {
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

bool GridLayout::handle_focus(const input_frame& input, ui::output_frame& output) {
  glm::ivec2 offset{0};
  if (input.pressed(ui::key::kRight)) {
    ++offset.x;
  }
  if (input.pressed(ui::key::kLeft)) {
    --offset.x;
  }
  if (input.pressed(ui::key::kDown)) {
    ++offset.y;
  }
  if (input.pressed(ui::key::kUp)) {
    --offset.y;
  }
  if (!size() || offset == glm::ivec2{0}) {
    return false;
  }

  auto columns = static_cast<std::size_t>(columns_);
  auto rows = (size() + columns - 1) / columns;
  auto final_columns = size() % columns ? size() % columns : columns;

  std::optional<std::size_t> focus_index;
  for (std::size_t i = 0; i < size(); ++i) {
    if ((*this)[i] == focused_child()) {
      focus_index = i;
    }
  }

  auto column = [&] { return *focus_index % columns; };
  auto row = [&] { return *focus_index / columns; };
  auto advance = [&]() -> bool {
    if (offset.x < 0) {
      if (!wrap_focus_ && !column()) {
        return false;
      }
      focus_index = (*focus_index + size() - 1) % size();
    } else if (offset.x > 0) {
      if (!wrap_focus_ &&
          (1 + column() == columns || (1 + row() == rows && 1 + column() == final_columns))) {
        return false;
      }
      focus_index = (*focus_index + 1) % size();
    }
    if (offset.y < 0) {
      if (!wrap_focus_ && !row()) {
        return offset.x;
      }
      focus_index = row()            ? *focus_index - columns
          : column() < final_columns ? size() - final_columns + column()
                                     : size() - final_columns - columns + column();
    } else if (offset.y > 0) {
      if (!wrap_focus_ && (1 + row() == rows || (2 + row() == rows && column() >= final_columns))) {
        return offset.x;
      }
      focus_index = 1 + row() == rows                      ? *focus_index + final_columns
          : 2 + row() == rows && column() >= final_columns ? *focus_index + columns + final_columns
                                                           : *focus_index + columns;
    }
    return true;
  };

  if (!focus_index) {
    if (offset.x >= 0) {
      focus_index = offset.y < 0 ? size() - final_columns : 0;
    } else {
      focus_index = offset.y < 0 ? size() - 1 : columns - 1;
    }
    if ((*this)[*focus_index]->focus()) {
      output.sounds.emplace(sound::kMenuClick);
      return true;
    }
  }
  auto start_index = *focus_index;
  while (true) {
    if (!advance()) {
      return false;
    }
    if ((*this)[*focus_index]->focus()) {
      output.sounds.emplace(sound::kMenuClick);
      return true;
    }
    if (*focus_index == start_index) {
      break;
    }
  }
  return false;
}

}  // namespace ii::ui