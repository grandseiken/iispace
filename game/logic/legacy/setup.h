#ifndef GAME_LOGIC_LEGACY_SETUP_H
#define GAME_LOGIC_LEGACY_SETUP_H
#include "game/logic/sim/setup.h"

namespace ii::legacy {

class LegacySimSetup : public SimSetup {
public:
  ~LegacySimSetup() override = default;
  game_parameters parameters(const initial_conditions&) const override;
  void initialise_systems(SimInterface& sim) override;
  ecs::entity_id start_game(const initial_conditions&, std::span<const std::uint32_t> ai_players,
                            SimInterface& sim) override;
  std::optional<input_frame> ai_think(const SimInterface& sim, ecs::handle h) override;
};

}  // namespace ii::legacy

#endif