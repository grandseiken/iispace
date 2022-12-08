#ifndef II_GAME_LOGIC_V0_OVERMIND_BIOME_H
#define II_GAME_LOGIC_V0_OVERMIND_BIOME_H
#include "game/common/random.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/overmind/wave_data.h"
#include <vector>

namespace ii {
class SimInterface;
namespace v0 {

class Biome {
public:
  virtual ~Biome() = default;

  virtual bool
  update_background(RandomEngine&, const background_input&, background_update&) const = 0;
  virtual std::vector<wave_data>
  get_wave_list(const initial_conditions&, std::uint32_t biome_index) const = 0;
  virtual void spawn_wave(SimInterface&, const wave_data&) const = 0;
};

const Biome* get_biome(run_biome biome);

}  // namespace v0
}  // namespace ii

#endif
