#ifndef II_GAME_CORE_IO_INPUT_ADAPTER_H
#define II_GAME_CORE_IO_INPUT_ADAPTER_H
#include "game/logic/sim/io/player.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ii {
class ReplayWriter;
namespace io {
class IoLayer;
}  // namespace io

class IoInputAdapter {
public:
  ~IoInputAdapter();
  IoInputAdapter(const io::IoLayer& io_layer);

  enum input_type : std::uint32_t {
    kNone = 0,
    kKeyboardMouse = 1,
    kController = 2,
  };

  std::uint32_t player_count() const {
    return player_count_;
  }
  void set_player_count(std::uint32_t players);
  void set_game_dimensions(const glm::uvec2& dimensions);
  input_type input_type_for(std::uint32_t player_index) const;
  std::vector<input_frame> get();
  void rumble(std::uint32_t player_index, std::uint16_t lf, std::uint16_t hf,
              std::uint32_t duration_ms) const;

private:
  const io::IoLayer& io_layer_;

  std::uint32_t player_count_ = 0;
  glm::uvec2 game_dimensions_ = {0, 0};

  std::vector<vec2> last_aim_;
  bool mouse_moving_ = false;
};

}  // namespace ii

#endif