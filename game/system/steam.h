#ifndef II_GAME_SYSTEM_STEAM_H
#define II_GAME_SYSTEM_STEAM_H
#include "game/system/system.h"

namespace ii {

class SteamSystem : public System {
public:
  ~SteamSystem() override;
  result<std::vector<std::string>> init(int argc, const char** argv) override;
};

}  // namespace ii

#endif