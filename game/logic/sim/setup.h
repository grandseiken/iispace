#ifndef GAME_LOGIC_SIM_SETUP_H
#define GAME_LOGIC_SIM_SETUP_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include <cstdint>
#include <optional>
#include <span>

namespace ii {
class SimInterface;

class SimSetup {
public:
  virtual ~SimSetup() = default;

  struct game_parameters {
    std::uint32_t fps = 0;
    vec2 dimensions{0};
  };

  virtual game_parameters parameters(const initial_conditions&) const = 0;
  virtual void initialise_systems(SimInterface& sim) = 0;
  // Returns global entity ID.
  virtual ecs::entity_id start_game(const initial_conditions&, SimInterface& sim) = 0;
  virtual void begin_tick(SimInterface& sim) = 0;
  virtual bool is_game_over(const SimInterface& sim) const = 0;
};

}  // namespace ii

#endif