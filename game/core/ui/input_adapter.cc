#include "game/core/ui/input_adapter.h"
#include "game/io/io.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace ii::ui {
namespace {
constexpr std::uint32_t kKeyRepeatDelayFrames = 20u;
constexpr std::uint32_t kKeyRepeatIntervalFrames = 5u;

template <typename T>
bool contains(std::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

bool is_navigation(key k) {
  return k == key::kUp || k == key::kDown || k == key::kLeft || k == key::kRight;
}

std::span<const io::keyboard::key> keys_for(key k) {
  using type = io::keyboard::key;
  static constexpr std::array accept = {type::kReturn};
  static constexpr std::array start = {type::kSpacebar};
  static constexpr std::array cancel = {type::kEscape};
  static constexpr std::array escape = {type::kEscape};
  static constexpr std::array up = {type::kW, type::kUArrow};
  static constexpr std::array down = {type::kS, type::kDArrow};
  static constexpr std::array left = {type::kA, type::kLArrow};
  static constexpr std::array right = {type::kD, type::kRArrow};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kStart:
    return start;
  case key::kEscape:
    return escape;
  case key::kUp:
    return up;
  case key::kDown:
    return down;
  case key::kLeft:
    return left;
  case key::kRight:
    return right;
  default:
    return {};
  }
}

std::span<const io::mouse::button> mouse_buttons_for(key k) {
  using type = io::mouse::button;
  static constexpr std::array click = {type::kL};
  static constexpr std::array cancel = {type::kR};

  switch (k) {
  case key::kClick:
    return click;
  case key::kCancel:
    return cancel;
  default:
    return {};
  }
}

std::span<const io::controller::button> controller_buttons_for(key k) {
  using type = io::controller::button;
  static constexpr std::array accept = {type::kA};
  static constexpr std::array cancel = {type::kB};
  static constexpr std::array start = {type::kStart, type::kGuide};
  static constexpr std::array up = {type::kDpadUp};
  static constexpr std::array down = {type::kDpadDown};
  static constexpr std::array left = {type::kDpadLeft};
  static constexpr std::array right = {type::kDpadRight};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kStart:
    return start;
  case key::kEscape:
    return {};
  case key::kUp:
    return up;
  case key::kDown:
    return down;
  case key::kLeft:
    return left;
  case key::kRight:
    return right;
  default:
    return {};
  }
}

void handle(input_frame& result, const io::mouse::frame& frame) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto buttons = mouse_buttons_for(k);
    for (auto b : buttons) {
      result.held(k) |= frame.button(b);
    }
    for (const auto& e : frame.button_events) {
      if (e.down && contains(buttons, e.button)) {
        result.pressed(k) = true;
      }
    }
  }
  result.mouse_delta = frame.cursor_delta;
  result.mouse_cursor = frame.cursor;
  result.mouse_scroll = frame.wheel_delta;
}

void handle(input_frame& result, const io::keyboard::frame& frame) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto keys = keys_for(k);
    for (auto kbk : keys) {
      result.held(k) |= frame.key(kbk);
    }
    for (const auto& e : frame.key_events) {
      if (e.down && e.key == io::keyboard::key::kSpacebar) {
        result.join_game_inputs.emplace_back(input_device_id::kbm());
      }
      if (e.down && contains(keys, e.key)) {
        result.pressed(k) = true;
        result.pad_navigation |= is_navigation(k);
      }
    }
  }
}

void handle(input_frame& result, const io::controller::frame& frame, std::size_t index,
            glm::ivec2& previous) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto buttons = controller_buttons_for(k);
    for (auto b : buttons) {
      result.held(k) |= frame.button(b);
    }
    for (const auto& e : frame.button_events) {
      if (e.down && e.button == io::controller::button::kStart) {
        result.join_game_inputs.emplace_back(input_device_id::controller(index));
      }
      if (e.down && contains(buttons, e.button)) {
        result.pressed(k) = true;
        result.pad_navigation |= is_navigation(k);
      }
    }
  }

  auto convert_axis = [](std::int16_t v) -> std::int32_t {
    auto f = std::abs(static_cast<float>(v)) / std::numeric_limits<std::int16_t>::max();
    f = std::round(4.f * std::clamp(2.f * f - 1.f, 0.f, 1.f));
    auto i = static_cast<std::int32_t>(f);
    return v >= 0 ? i : -i;
  };

  if (auto x = convert_axis(frame.axis(io::controller::axis::kRX)); x) {
    if (!result.controller_scroll) {
      result.controller_scroll = {0, 0};
    }
    result.controller_scroll->x += x;
  }

  if (auto y = convert_axis(frame.axis(io::controller::axis::kRY)); y) {
    if (!result.controller_scroll) {
      result.controller_scroll = {0, 0};
    }
    result.controller_scroll->y += y;
  }

  auto x = convert_axis(frame.axis(io::controller::axis::kLX));
  auto y = convert_axis(frame.axis(io::controller::axis::kLY));
  result.pad_navigation |= abs(x) > 1 || abs(y) > 1;
  result.held(ui::key::kLeft) |= x < -1;
  result.held(ui::key::kRight) |= x > 1;
  result.held(ui::key::kUp) |= y < -1;
  result.held(ui::key::kDown) |= y > 1;
  result.pressed(ui::key::kLeft) |= x < -1 && previous.x >= -1;
  result.pressed(ui::key::kRight) |= x > 1 && previous.x <= 1;
  result.pressed(ui::key::kUp) |= y < -1 && previous.y >= -1;
  result.pressed(ui::key::kDown) |= y > 1 && previous.y <= 1;
  previous.x = x;
  previous.y = y;
}

}  // namespace

InputAdapter::InputAdapter(io::IoLayer& io_layer) : io_layer_{io_layer} {}

multi_input_frame InputAdapter::ui_frame(bool controller_change) {
  multi_input_frame input;
  std::uint32_t size = kbm_assignment_ ? *kbm_assignment_ + 1 : 0;
  for (const auto& pair : controller_assignments_) {
    size = std::max(size, 1 + pair.second);
  }
  input.assignments.resize(size);
  input.global.controller_change = controller_change;
  for (auto& frame : input.assignments) {
    frame.controller_change = controller_change;
  }

  if (controller_change) {
    prev_controller_.clear();
    for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
      prev_controller_.emplace_back(glm::ivec2{0});
    }
  }
  if (kbm_assignment_) {
    handle(input.assignments[*kbm_assignment_], io_layer_.keyboard_frame());
    handle(input.assignments[*kbm_assignment_], io_layer_.mouse_frame());
    handle(input.global, io_layer_.mouse_frame());
  } else {
    handle(input.global, io_layer_.keyboard_frame());
    handle(input.global, io_layer_.mouse_frame());
  }
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    if (auto it = controller_assignments_.find(i); it != controller_assignments_.end()) {
      handle(input.assignments[it->second], io_layer_.controller_frame(i), i, prev_controller_[i]);
    } else {
      handle(input.global, io_layer_.controller_frame(i), i, prev_controller_[i]);
    }
  }

  auto do_repeats = [&](input_frame& frame, key_repeat_data& data) {
    for (std::size_t i = 0; i < static_cast<std::size_t>(ui::key::kMax); ++i) {
      if (frame.key_held[i] && is_navigation(static_cast<ui::key>(i))) {
        ++data[i];
      } else {
        data[i] = 0;
      }
      if (data[i] >= kKeyRepeatDelayFrames &&
          !((data[i] - kKeyRepeatDelayFrames) % kKeyRepeatIntervalFrames)) {
        frame.key_pressed[i] = true;
      }
    }
  };
  assignment_repeat_data_.resize(size);
  do_repeats(input.global, global_repeat_data);
  for (std::uint32_t i = 0; i < size; ++i) {
    do_repeats(input.assignments[i], assignment_repeat_data_[i]);
  }

  auto mouse_moved = [](const input_frame& frame) {
    return frame.mouse_delta && *frame.mouse_delta != glm::ivec2{0};
  };
  if ((!kbm_assignment_ && mouse_moved(input.global)) ||
      (kbm_assignment_ && mouse_moved(input.assignments[*kbm_assignment_]))) {
    show_cursor_ = true;
  } else if ((!kbm_assignment_ && input.global.pad_navigation) ||
             (kbm_assignment_ && input.assignments[*kbm_assignment_].pad_navigation)) {
    show_cursor_ = false;
  }
  input.show_cursor = show_cursor_;
  return input;
}

void InputAdapter::clear_assignments() {
  kbm_assignment_.reset();
  controller_assignments_.clear();
  assignment_repeat_data_.clear();
}

void InputAdapter::assign_input_device(input_device_id id, std::uint32_t assignment) {
  if (id.controller_index) {
    controller_assignments_[*id.controller_index] = assignment;
  } else {
    kbm_assignment_ = assignment;
  }
  if (assignment < assignment_repeat_data_.size()) {
    assignment_repeat_data_[assignment].fill(0);
  }
}

bool InputAdapter::is_assigned(input_device_id id) {
  if (id.controller_index) {
    return controller_assignments_.contains(*id.controller_index);
  }
  return kbm_assignment_.has_value();
}

void InputAdapter::unassign(std::uint32_t assignment) {
  if (kbm_assignment_ == assignment) {
    kbm_assignment_.reset();
  }
  for (auto it = controller_assignments_.begin(); it != controller_assignments_.end();) {
    if (it->second == assignment) {
      it = controller_assignments_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace ii::ui