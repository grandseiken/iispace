#ifndef IISPACE_GAME_CORE_IO_INPUT_ADAPTER_H
#define IISPACE_GAME_CORE_IO_INPUT_ADAPTER_H
#include "game/logic/sim/sim_io.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ii {
class ReplayWriter;
namespace io {
class IoLayer;
}  // namespace io

class IoInputAdapter : public InputAdapter {
public:
  ~IoInputAdapter();
  IoInputAdapter(const io::IoLayer& io_layer, ReplayWriter* replay_writer);

  enum input_type : std::uint32_t {
    kNone = 0,
    kKeyboardMouse = 1,
    kController = 2,
  };

  void set_player_count(std::uint32_t players);
  void set_game_dimensions(const glm::uvec2& dimensions);
  input_type input_type_for(std::uint32_t player_index) const;
  std::vector<input_frame> get() override;

private:
  const io::IoLayer& io_layer_;
  ReplayWriter* replay_writer_ = nullptr;

  std::uint32_t player_count_ = 0;
  glm::uvec2 game_dimensions_ = {0, 0};

  std::vector<vec2> last_aim_;
  bool mouse_moving_ = false;
};

}  // namespace ii

#endif