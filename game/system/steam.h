#ifndef II_GAME_SYSTEM_STEAM_H
#define II_GAME_SYSTEM_STEAM_H
#include "game/system/system.h"
#include <memory>

namespace ii {

class SteamSystem : public System {
public:
  SteamSystem();
  ~SteamSystem() override;
  result<std::vector<std::string>> init() override;
  void tick() override;
  const std::vector<event>& events() const override;

  bool supports_networked_multiplayer() const override;
  ustring local_username() const override;
  const std::vector<friend_info>& friend_list() const override;

  async_result<void> create_lobby() override;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif