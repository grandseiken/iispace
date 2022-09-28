#ifndef II_GAME_SYSTEM_DEFAULT_H
#define II_GAME_SYSTEM_DEFAULT_H
#include "game/system/system.h"

namespace ii {

class DefaultSystem : public System {
public:
  ~DefaultSystem() override = default;
  result<std::vector<std::string>> init(int argc, const char** argv) override {
    std::vector<std::string> result;
    for (int i = 1; i < argc; ++i) {
      result.emplace_back(argv[i]);
    }
    return result;
  }

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
  std::optional<avatar_info> avatar(std::uint64_t) const override { return std::nullopt; }

  void leave_lobby() override {}
  async_result<void> create_lobby() override { return {unexpected("unsupported")}; }
  async_result<void> join_lobby(std::uint64_t) override { return {unexpected("unsupported")}; }
  std::optional<lobby_info> current_lobby() const override { return std::nullopt; }
  void show_invite_dialog() const override {}

  std::optional<session_info> session(std::uint64_t) const override { return std::nullopt; }
  void send_to(std::uint64_t, const send_message&) override {}
  void send_to_host(const send_message&) override {}
  void broadcast(const send_message&) override {}
  void receive(std::uint32_t, std::vector<received_message>&) override {}
};

}  // namespace ii

#endif