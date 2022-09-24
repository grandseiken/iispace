#ifndef II_GAME_CORE_SIM_INPUT_ADAPTER_H
#define II_GAME_CORE_SIM_INPUT_ADAPTER_H
#include "game/core/ui/input.h"
#include "game/logic/sim/io/player.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ii {
namespace data {
class ReplayWriter;
}  // namespace data
namespace io {
class IoLayer;
}  // namespace io

// TODO: handle controllers changed.
class SimInputAdapter {
public:
  ~SimInputAdapter();
  SimInputAdapter(const io::IoLayer& io_layer, std::span<const ui::input_device_id> input_devices);

  std::uint32_t player_count() const { return input_.size(); }
  void set_game_dimensions(const glm::uvec2& dimensions);
  std::vector<input_frame> get();
  void rumble(std::uint32_t player_index, std::uint16_t lf, std::uint16_t hf,
              std::uint32_t duration_ms) const;

private:
  const io::IoLayer& io_layer_;

  struct player_input {
    ui::input_device_id device;
    vec2 last_aim{0, 0};
  };

  std::vector<player_input> input_;
  glm::uvec2 game_dimensions_ = {0, 0};
  bool mouse_moving_ = false;
};

}  // namespace ii

#endif