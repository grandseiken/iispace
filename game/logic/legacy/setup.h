#ifndef GAME_LOGIC_LEGACY_SETUP_H
#define GAME_LOGIC_LEGACY_SETUP_H
#include "game/logic/sim/setup.h"

namespace ii::legacy {

class LegacySimSetup : public SimSetup {
public:
  ~LegacySimSetup() override = default;
  game_parameters parameters(const initial_conditions&) const override;
  void initialise_systems(SimInterface& sim) override;
  ecs::entity_id start_game(const initial_conditions&, SimInterface& sim) override;
  void begin_tick(SimInterface& sim) override;
  bool is_game_over(const SimInterface& sim) const override;
};

}  // namespace ii::legacy

#endif