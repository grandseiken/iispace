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
    const static std::vector<event> kEmpty;
    return kEmpty;
  }

  bool supports_networked_multiplayer() const override { return false; }
  ustring local_username() const override { return ustring::ascii("Player"); }
  const std::vector<friend_info>& friend_list() const override {
    const static std::vector<friend_info> kEmpty;
    return kEmpty;
  }

  void create_lobby(callback<void> cb) override { cb(unexpected("unsupported")); }
};

}  // namespace ii

#endif