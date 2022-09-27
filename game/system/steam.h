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
  user_info local_user() const override;
  const std::vector<friend_info>& friend_list() const override;
  std::optional<avatar_info> avatar(std::uint64_t user_id) const override;

  void leave_lobby() override;
  async_result<void> create_lobby() override;
  async_result<void> join_lobby(std::uint64_t lobby_id) override;
  std::optional<lobby_info> current_lobby() const override;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif