#ifndef II_GAME_SYSTEM_SYSTEM_H
#define II_GAME_SYSTEM_SYSTEM_H
#include "game/common/result.h"
#include "game/common/ustring.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ii {

class System {
public:
  template <typename T>
  using callback = std::function<void(result<T>)>;

  enum class event_type {
    kNone,
    kOverlayActivated,
  };

  struct event {
    event_type type = event_type::kNone;
  };

  struct friend_info {
    std::uint64_t id = 0;
    ustring username = ustring::ascii("");
    bool in_game = false;
    std::optional<std::uint64_t> lobby_id;
  };

  virtual ~System() = default;
  virtual result<std::vector<std::string>> init() = 0;

  virtual void tick() = 0;
  virtual const std::vector<event>& events() const = 0;

  virtual bool supports_networked_multiplayer() const = 0;
  virtual ustring local_username() const = 0;
  virtual const std::vector<friend_info>& friend_list() const = 0;

  virtual void create_lobby(callback<void> cb) = 0;
};

std::unique_ptr<System> create_system();

}  // namespace ii

#endif