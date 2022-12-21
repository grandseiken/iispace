#include "game/core/sim/input_adapter.h"
#include "game/core/ui/input.h"
#include "game/io/io.h"
#include <limits>
#include <optional>

namespace ii {
namespace {

vec2 controller_stick(std::int16_t x, std::int16_t y, bool should_normalise) {
  static constexpr fixed inner_deadzone = fixed_c::tenth * 3;
  static constexpr fixed outer_deadzone = fixed_c::tenth;
  vec2 v{fixed::from_internal(static_cast<std::int64_t>(x) << 17),
         fixed::from_internal(static_cast<std::int64_t>(y) << 17)};
  auto l = length(v);
  if (l < inner_deadzone) {
    return vec2{0};
  }
  v = normalise(v);
  if (should_normalise || l > 1 - outer_deadzone) {
    return v;
  }
  l = (l - inner_deadzone) / (1 - outer_deadzone - inner_deadzone);
  return v * l;
}

vec2 kbm_move_velocity(const io::keyboard::frame& frame) {
  bool u = frame.key(io::keyboard::key::kUArrow) || frame.key(io::keyboard::key::kW);
  bool d = frame.key(io::keyboard::key::kDArrow) || frame.key(io::keyboard::key::kS);
  bool l = frame.key(io::keyboard::key::kLArrow) || frame.key(io::keyboard::key::kA);
  bool r = frame.key(io::keyboard::key::kRArrow) || frame.key(io::keyboard::key::kD);
  auto v = vec2{0, -1} * fixed{u} + vec2{0, 1} * fixed{d} + vec2{-1, 0} * fixed{l} +
      vec2{1, 0} * fixed{r};
  if (frame.key(io::keyboard::key::kLShift) || frame.key(io::keyboard::key::kRShift)) {
    v = normalise(v) / 2;
  }
  return v;
}

vec2 kbm_fire_target(const io::mouse::frame& frame, const uvec2& game_dimensions,
                     const uvec2& screen_dimensions) {
  auto screen = static_cast<fvec2>(screen_dimensions);
  auto game = static_cast<fvec2>(game_dimensions);
  auto screen_aspect = screen.x / screen.y;
  auto game_aspect = game.x / game.y;
  auto scale = screen_aspect > game_aspect ? fvec2{game_aspect / screen_aspect, 1.f}
                                           : fvec2{1.f, screen_aspect / game_aspect};

  auto screen_cursor = static_cast<fvec2>(
      frame.cursor.value_or(ivec2{screen_dimensions.x / 2, screen_dimensions.y / 2}));
  auto cursor = screen_cursor / screen - (fvec2{1.f, 1.f} - scale) / 2.f;
  cursor *= game / scale;
  cursor.x = std::max(0.f, std::min(game.x, cursor.x));
  cursor.y = std::max(0.f, std::min(game.y, cursor.y));
  return vec2{static_cast<std::int32_t>(cursor.x), static_cast<std::int32_t>(cursor.y)};
}

std::uint32_t
kbm_keys(const io::keyboard::frame& keyboard_frame, const io::mouse::frame& mouse_frame) {
  std::uint32_t result = 0;
  if (mouse_frame.button(io::mouse::button::kL)) {
    result |= input_frame::key::kFire;
  }
  if (mouse_frame.button(io::mouse::button::kR)) {
    result |= input_frame::key::kBomb;
  }
  if (mouse_frame.button(io::mouse::button::kM) || keyboard_frame.key(io::keyboard::key::kLCtrl) ||
      keyboard_frame.key(io::keyboard::key::kRCtrl)) {
    result |= input_frame::key::kSuper;
  }
  if (keyboard_frame.key(io::keyboard::key::kReturn) ||
      mouse_frame.button(io::mouse::button::kX1)) {
    result |= input_frame::key::kClick;
  }
  return result;
}

vec2 controller_move_velocity(const io::controller::frame& frame) {
  bool u = frame.button(io::controller::button::kDpadUp);
  bool d = frame.button(io::controller::button::kDpadDown);
  bool l = frame.button(io::controller::button::kDpadLeft);
  bool r = frame.button(io::controller::button::kDpadRight);
  auto v = vec2{0, -1} * fixed{u} + vec2{0, 1} * fixed{d} + vec2{-1, 0} * fixed{l} +
      vec2{1, 0} * fixed{r};
  if (v != vec2{0}) {
    return v;
  }
  return controller_stick(frame.axis(io::controller::axis::kLX),
                          frame.axis(io::controller::axis::kLY), false);
}

vec2 controller_fire_target(const io::controller::frame& frame) {
  return controller_stick(frame.axis(io::controller::axis::kRX),
                          frame.axis(io::controller::axis::kRY), true);
}

std::uint32_t controller_keys(const io::controller::frame& frame) {
  static constexpr auto kTriggerThreshold = std::numeric_limits<std::int16_t>::max() / 2;
  std::uint32_t result = 0;
  if (frame.button(io::controller::button::kRShoulder)) {
    result |= input_frame::key::kFire;
  }
  if (frame.axis(io::controller::axis::kLT) >= kTriggerThreshold ||
      frame.button(io::controller::button::kLShoulder)) {
    result |= input_frame::key::kBomb;
  }
  if (frame.axis(io::controller::axis::kRT) >= kTriggerThreshold) {
    result |= input_frame::key::kSuper;
  }
  if (frame.button(io::controller::button::kA)) {
    result |= input_frame::key::kClick;
  }
  return result;
}

}  // namespace

SimInputAdapter::~SimInputAdapter() = default;
SimInputAdapter::SimInputAdapter(const io::IoLayer& io_layer,
                                 std::span<const ui::input_device_id> input_devices)
: io_layer_{io_layer} {
  for (const auto& input_device : input_devices) {
    input_.emplace_back().device = input_device;
  }
}

void SimInputAdapter::set_game_dimensions(const uvec2& dimensions) {
  game_dimensions_ = dimensions;
}

std::vector<input_frame> SimInputAdapter::get() {
  auto mouse_frame = io_layer_.mouse_frame();
  auto keyboard_frame = io_layer_.keyboard_frame();
  std::vector<io::controller::frame> controller_frames;
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    controller_frames.emplace_back(io_layer_.controller_frame(i));
  }

  std::vector<input_frame> frames;
  bool single_player = input_.size() <= 1u;
  for (auto& input : input_) {
    auto& frame = frames.emplace_back();
    io::controller::frame* controller = input.device.controller_index
        ? &controller_frames[*input.device.controller_index]
        : nullptr;
    bool kbm = !controller || single_player;
    auto set_single_player_device = [&](ui::input_device_id id) {
      if (single_player) {
        input.device = id;
      }
    };

    auto for_controllers = [&](auto&& f) {
      if (single_player) {
        for (std::size_t i = 0; i < controller_frames.size(); ++i) {
          f(i, controller_frames[i]);
        }
      } else if (controller) {
        f(*input.device.controller_index, *controller);
      }
    };

    if (kbm) {
      frame.velocity = kbm_move_velocity(keyboard_frame);
      if (frame.velocity != vec2{0}) {
        set_single_player_device(ui::input_device_id::kbm());
      }
    }
    for_controllers([&](std::size_t index, const auto& controller_frame) {
      if (frame.velocity == vec2{0}) {
        frame.velocity = controller_move_velocity(controller_frame);
        if (frame.velocity != vec2{0}) {
          set_single_player_device(ui::input_device_id::controller(index));
        }
      }
    });

    for_controllers([&](std::size_t index, const auto& controller_frame) {
      auto v = controller_fire_target(controller_frame);
      if (!frame.target_relative && v != vec2{0}) {
        if (kbm) {
          mouse_moving_ = false;
        }
        frame.target_relative = input.last_aim = normalise(v) * 48;
        frame.keys |= input_frame::key::kFire;
        set_single_player_device(ui::input_device_id::controller(index));
      }
    });
    if (kbm && !frame.target_relative) {
      if (mouse_frame.cursor_delta != ivec2{0, 0}) {
        mouse_moving_ = true;
        set_single_player_device(ui::input_device_id::kbm());
      }
      if (mouse_moving_) {
        frame.target_absolute =
            kbm_fire_target(mouse_frame, game_dimensions_, io_layer_.dimensions());
      }
    }
    if (!frame.target_absolute && !frame.target_relative) {
      if ((controller || single_player) && input.last_aim != vec2{0}) {
        frame.target_relative = input.last_aim;
      } else {
        frame.target_relative = vec2{48, 0};
      }
    }

    if (kbm) {
      auto keys = kbm_keys(keyboard_frame, mouse_frame);
      frame.keys |= keys;
      if (keys) {
        set_single_player_device(ui::input_device_id::kbm());
      }
    }
    for_controllers([&](std::size_t index, const auto& controller_frame) {
      auto keys = controller_keys(controller_frame);
      frame.keys |= keys;
      if (keys) {
        set_single_player_device(ui::input_device_id::controller(index));
      }
    });
  }
  return frames;
}

void SimInputAdapter::rumble(std::uint32_t player_index, std::uint16_t lf, std::uint16_t hf,
                             std::uint32_t duration_ms) const {
  if (player_index < input_.size() && input_[player_index].device.controller_index) {
    io_layer_.controller_rumble(*input_[player_index].device.controller_index, lf, hf, duration_ms);
  }
}

}  // namespace ii
