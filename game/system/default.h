#ifndef II_GAME_SYSTEM_DEFAULT_H
#define II_GAME_SYSTEM_DEFAULT_H
#include "game/system/system.h"

namespace ii {

class DefaultSystem : public System {
public:
  ~DefaultSystem() override = default;
  result<std::vector<std::string>> init() override { return {}; }

  void tick() override {}
  const std::vector<event>& events() const override {
    static const std::vector<event> kEmpty;
    return kEmpty;
  }

  bool supports_networked_multiplayer() const override { return false; }
  user_info local_user() const override { return {0, ustring::ascii("Player")}; }
  const std::vector<friend_info>& friend_list() const override {
    static const std::vector<friend_info> kEmpty;
    return kEmpty;
  }

  void leave_lobby() override {}
  async_result<void> create_lobby() override { return {unexpected("unsupported")}; }
  async_result<void> join_lobby(std::uint64_t lobby_id) override {
    return {unexpected("unsupported")};
  }
  std::optional<lobby_info> current_lobby() const override { return std::nullopt; }
  const std::vector<lobby_info>& lobby_results() const override {
    static const std::vector<lobby_info> kEmpty;
    return kEmpty;
  }
};

}  // namespace ii

#endif