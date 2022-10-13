#ifndef GAME_LOGIC_V0_SETUP_H
#define GAME_LOGIC_V0_SETUP_H
#include "game/logic/sim/setup.h"

namespace ii::v0 {

class V0SimSetup : public SimSetup {
public:
  ~V0SimSetup() override = default;
  game_parameters parameters(const initial_conditions&) const override;
  void initialise_systems(SimInterface& sim) override;
  ecs::entity_id start_game(const initial_conditions& conditions, SimInterface& sim) override;
};

}  // namespace ii::v0

#endif