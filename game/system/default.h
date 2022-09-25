#ifndef II_GAME_SYSTEM_DEFAULT_H
#define II_GAME_SYSTEM_DEFAULT_H
#include "game/system/system.h"

namespace ii {

class DefaultSystem : public System {
public:
  ~DefaultSystem() override = default;
  result<std::vector<std::string>> init() override { return {}; }

  bool supports_network_multiplayer() const override { return false; }
  ustring local_username() const override { return ustring::ascii("Player"); }
  std::size_t friends_in_game() const override { return 0; }
};

}  // namespace ii

#endif