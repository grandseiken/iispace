#include "game/core/io_input_adapter.h"
#include "game/data/replay.h"
#include "game/io/io.h"
#include <optional>

namespace ii {
namespace {

struct player_input {
  bool kbm = false;
  std::optional<std::size_t> controller;
};

player_input
assign_input(std::uint32_t player_index, std::uint32_t player_count, std::size_t controller_count) {
  player_input input;
  if (player_index < controller_count) {
    input.controller = player_index;
  }
  input.kbm = (player_count <= controller_count && 1 + player_index == player_count) ||
      (player_count > controller_count && player_index == controller_count);
  return input;
}

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

  auto cursor = static_cast<glm::vec2>(frame.cursor) / screen - (glm::vec2{1.f, 1.f} - scale) / 2.f;
  cursor *= game / scale;
  cursor.x = std::max(0.f, std::min(game.x, cursor.x));
  cursor.y = std::max(0.f, std::min(game.y, cursor.y));
  return vec2{cursor.x, cursor.y};
}

std::int32_t
kbm_keys(const io::keyboard::frame& keyboard_frame, const io::mouse::frame& mouse_frame) {
  std::int32_t result = 0;
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

std::int32_t controller_keys(const io::controller::frame& frame) {
  std::int32_t result = 0;
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

IoInputAdapter::~IoInputAdapter() = default;
IoInputAdapter::IoInputAdapter(const io::IoLayer& io_layer, ReplayWriter* replay_writer)
: io_layer_{io_layer}, replay_writer_{replay_writer} {}

void IoInputAdapter::set_player_count(std::uint32_t players) {
  player_count_ = players;
  last_aim_.clear();
  for (std::uint32_t i = 0; i < players; ++i) {
    last_aim_.emplace_back(vec2{0});
  }
}

void IoInputAdapter::set_game_dimensions(const glm::uvec2& dimensions) {
  game_dimensions_ = dimensions;
}

IoInputAdapter::input_type IoInputAdapter::input_type_for(std::uint32_t player_index) const {
  input_type result = input_type::kNone;
  auto input = assign_input(player_index, player_count_, io_layer_.controllers());
  if (input.controller) {
    result = input_type::kController;
  }
  if (input.kbm) {
    result = static_cast<input_type>(result | input_type::kKeyboardMouse);
  }
  return result;
}

std::vector<input_frame> IoInputAdapter::get() {
  auto mouse_frame = io_layer_.mouse_frame();
  auto keyboard_frame = io_layer_.keyboard_frame();
  std::vector<io::controller::frame> controller_frames;
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    controller_frames.emplace_back(io_layer_.controller_frame(i));
  }

  std::vector<input_frame> result;
  for (std::uint32_t i = 0; i < player_count_; ++i) {
    auto& frame = result.emplace_back();
    auto input = assign_input(i, player_count_, io_layer_.controllers());
    io::controller::frame* controller =
        input.controller ? &controller_frames[*input.controller] : nullptr;

    if (input.kbm) {
      frame.velocity = kbm_move_velocity(keyboard_frame);
    }
    if (controller && frame.velocity == vec2{0}) {
      frame.velocity = controller_move_velocity(*controller);
    }

    if (controller) {
      auto v = controller_fire_target(*controller);
      if (v != vec2{0}) {
        if (input.kbm) {
          mouse_moving_ = false;
        }
        frame.target_relative = last_aim_[i] = normalise(v) * 48;
        frame.keys |= input_frame::key::kFire;
      }
    }
    if (input.kbm && !frame.target_relative) {
      if (mouse_frame.cursor_delta != glm::ivec2{0, 0}) {
        mouse_moving_ = true;
      }
      if (mouse_moving_) {
        frame.target_absolute =
            kbm_fire_target(mouse_frame, game_dimensions_, io_layer_.dimensions());
      }
    }
    if (!frame.target_absolute && !frame.target_relative) {
      if (controller && last_aim_[i] != vec2{0}) {
        frame.target_relative = last_aim_[i];
      } else {
        frame.target_relative = vec2{48, 0};
      }
    }

    if (input.kbm) {
      frame.keys |= kbm_keys(keyboard_frame, mouse_frame);
    }
    if (controller) {
      frame.keys |= controller_keys(*controller);
    }
  }
  if (replay_writer_) {
    for (const auto& frame : result) {
      replay_writer_->add_input_frame(frame);
    }
  }
  return result;
}

}  // namespace ii
