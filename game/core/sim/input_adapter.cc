#include "game/core/sim/input_adapter.h"
#include "game/core/ui/input.h"
#include "game/io/io.h"
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
  return vec2{0, -1} * fixed{u} + vec2{0, 1} * fixed{d} + vec2{-1, 0} * fixed{l} +
      vec2{1, 0} * fixed{r};
}

vec2 kbm_fire_target(const io::mouse::frame& frame, const glm::uvec2& game_dimensions,
                     const glm::uvec2& screen_dimensions) {
  auto screen = static_cast<glm::vec2>(screen_dimensions);
  auto game = static_cast<glm::vec2>(game_dimensions);
  auto screen_aspect = screen.x / screen.y;
  auto game_aspect = game.x / game.y;
  auto scale = screen_aspect > game_aspect ? glm::vec2{game_aspect / screen_aspect, 1.f}
                                           : glm::vec2{1.f, screen_aspect / game_aspect};

  auto screen_cursor = static_cast<glm::vec2>(
      frame.cursor.value_or(glm::ivec2{screen_dimensions.x / 2, screen_dimensions.y / 2}));
  auto cursor = screen_cursor / screen - (glm::vec2{1.f, 1.f} - scale) / 2.f;
  cursor *= game / scale;
  cursor.x = std::max(0.f, std::min(game.x, cursor.x));
  cursor.y = std::max(0.f, std::min(game.y, cursor.y));
  return vec2{static_cast<std::int32_t>(cursor.x), static_cast<std::int32_t>(cursor.y)};
}

std::uint32_t
kbm_keys(const io::keyboard::frame& keyboard_frame, const io::mouse::frame& mouse_frame) {
  std::uint32_t result = 0;
  if (keyboard_frame.key(io::keyboard::key::kZ) || keyboard_frame.key(io::keyboard::key::kLCtrl) ||
      keyboard_frame.key(io::keyboard::key::kRCtrl) || mouse_frame.button(io::mouse::button::kL)) {
    result |= input_frame::key::kFire;
  }
  if (keyboard_frame.key(io::keyboard::key::kX) ||
      keyboard_frame.key(io::keyboard::key::kSpacebar) ||
      mouse_frame.button(io::mouse::button::kR)) {
    result |= input_frame::key::kBomb;
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
  std::uint32_t result = 0;
  if (frame.button(io::controller::button::kA) ||
      frame.button(io::controller::button::kRShoulder)) {
    result |= input_frame::key::kFire;
  }
  if (frame.button(io::controller::button::kB) ||
      frame.button(io::controller::button::kLShoulder)) {
    result |= input_frame::key::kBomb;
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

void SimInputAdapter::set_game_dimensions(const glm::uvec2& dimensions) {
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
    auto for_controllers = [&](auto&& f) {
      if (single_player) {
        for (const auto& controller_frame : controller_frames) {
          f(controller_frame);
        }
      } else if (controller) {
        f(*controller);
      }
    };

    if (kbm) {
      frame.velocity = kbm_move_velocity(keyboard_frame);
    }
    for_controllers([&](const auto& controller_frame) {
      if (frame.velocity == vec2{0}) {
        frame.velocity = controller_move_velocity(controller_frame);
      }
    });

    for_controllers([&](const auto& controller_frame) {
      auto v = controller_fire_target(controller_frame);
      if (!frame.target_relative && v != vec2{0}) {
        if (kbm) {
          mouse_moving_ = false;
        }
        frame.target_relative = input.last_aim = normalise(v) * 48;
        frame.keys |= input_frame::key::kFire;
      }
    });
    if (kbm && !frame.target_relative) {
      if (mouse_frame.cursor_delta != glm::ivec2{0, 0}) {
        mouse_moving_ = true;
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
      frame.keys |= kbm_keys(keyboard_frame, mouse_frame);
    }
    for_controllers(
        [&](const auto& controller_frame) { frame.keys |= controller_keys(controller_frame); });
  }
  return frames;
}

void SimInputAdapter::rumble(std::uint32_t player_index, std::uint16_t lf, std::uint16_t hf,
                             std::uint32_t duration_ms) const {
  // TODO: rumble in single-player if they have been using a controller.
  if (player_index < input_.size() && input_[player_index].device.controller_index) {
    io_layer_.controller_rumble(*input_[player_index].device.controller_index, lf, hf, duration_ms);
  }
}

}  // namespace ii
