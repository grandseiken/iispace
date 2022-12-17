#ifndef II_GAME_LOGIC_V0_PLAYER_POWERUP_H
#define II_GAME_LOGIC_V0_PLAYER_POWERUP_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/render/data/panel.h"
#include <vector>

namespace ii {
class SimInterface;
struct Transform;
namespace v0 {
ecs::handle spawn_player_bubble(SimInterface& sim, ecs::handle player);
void spawn_shield_powerup(SimInterface&, const vec2& position);
void spawn_bomb_powerup(SimInterface&, const vec2& position);

void render_player_name_panel(std::uint32_t player_number, const Transform& transform,
                              std::vector<render::combo_panel>& output, const SimInterface& sim);
}  // namespace v0
}  // namespace ii

#endif
