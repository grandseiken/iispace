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

}  // namespace ii