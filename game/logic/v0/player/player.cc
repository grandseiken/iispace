#include "game/logic/v0/player/player.h"

namespace ii::v0 {

void spawn_player(SimInterface& /* sim */, const vec2& /* position */,
                  std::uint32_t /* player_number */, bool /* is_ai */) {
  // TODO
}

std::optional<input_frame> ai_think(const SimInterface& /* sim */, ecs::handle /* h */) {
  return std::nullopt;  // TODO
}

}  // namespace ii::v0
