#include "game/logic/v0/components.h"
#include "game/logic/sim/sim_interface.h"

namespace ii::v0 {

void GlobalData::pre_update(SimInterface& sim) {
  non_wall_enemy_count = sim.index().count<Enemy>() - sim.index().count<WallTag>();
}

}  // namespace ii::v0