#ifndef II_GAME_SYSTEM_STEAM_H
#define II_GAME_SYSTEM_STEAM_H
#include "game/system/system.h"

namespace ii {

class SteamSystem : public System {
public:
  ~SteamSystem() override;
  result<std::vector<std::string>> init() override;

  bool supports_network_multiplayer() const override;
  ustring local_username() const override;
  std::size_t friends_in_game() const override;
};

}  // namespace ii

#endif