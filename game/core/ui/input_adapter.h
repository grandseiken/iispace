#ifndef II_GAME_CORE_UI_INPUT_ADAPTER_H
#define II_GAME_CORE_UI_INPUT_ADAPTER_H
#include "game/common/math.h"
#include "game/core/ui/input.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii::io {
class IoLayer;
}  // namespace ii::io

namespace ii::ui {

class InputAdapter {
public:
  InputAdapter(io::IoLayer& io_layer);
  multi_input_frame ui_frame(bool controller_change);

  // TODO: need some way of preserving assignments on controller change; will need
  // support from IoLayer.
  void clear_assignments();
  void assign_input_device(input_device_id id, std::uint32_t assignment);
  void unassign(std::uint32_t assignment);

  std::optional<std::uint32_t> assignment(input_device_id);
  bool is_assigned(input_device_id);

private:
  io::IoLayer& io_layer_;
  std::optional<std::uint32_t> kbm_assignment_;
  std::unordered_map<std::size_t, std::uint32_t> controller_assignments_;

  using key_repeat_data = std::array<std::uint32_t, static_cast<std::size_t>(key::kMax)>;
  key_repeat_data global_repeat_data = {0u};
  std::vector<key_repeat_data> assignment_repeat_data_;
  std::vector<ivec2> prev_controller_;
  bool show_cursor_ = true;
};

}  // namespace ii::ui

#endif