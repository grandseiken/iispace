#include "game/logic/v0/lib/components.h"

namespace ii::v0 {

void add(ecs::handle h, const AiFocusTag& v) {
  h.add(v);
}
void add(ecs::handle h, const AiClickTag& v) {
  h.add(v);
}
void add(ecs::handle h, const GlobalData& v) {
  h.add(v);
}
void add(ecs::handle h, const DropTable& v) {
  h.add(v);
}
void add(ecs::handle h, const ColourOverride& v) {
  h.add(v);
}
void add(ecs::handle h, const Physics& v) {
  h.add(v);
}
void add(ecs::handle h, const EnemyStatus& v) {
  h.add(v);
}

}  // namespace ii::v0
