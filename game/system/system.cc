#include "game/system/system.h"
#include "game/system/default.h"
#ifdef II_STEAM_SYSTEM
#include "game/system/steam.h"
#endif

namespace ii {

std::unique_ptr<System> create_system() {
#ifdef II_STEAM_SYSTEM
  return std::make_unique<SteamSystem>();
#else
  return std::make_unique<DefaultSystem>();
#endif
}

std::optional<System::event> event_triggered(const System& system, System::event_type type) {
  for (const auto& e : system.events()) {
    if (e.type == type) {
      return e;
    }
  }
  return std::nullopt;
}

}  // namespace ii