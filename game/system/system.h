#ifndef II_GAME_SYSTEM_SYSTEM_H
#define II_GAME_SYSTEM_SYSTEM_H
#include "game/common/async.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ii {

class System {
public:
  enum class event_type {
    kNone,
    kOverlayActivated,
    // Lobby join triggered from out-of-game (e.g. Steam invite).
    kLobbyJoinRequested,
    // Someone entered or left the lobby we are currently in.
    kLobbyMemberEntered,
    kLobbyMemberLeft,
    // We were disconnected from the lobby for some reason.
    kLobbyDisconnected,
  };

  struct event {
    event_type type = event_type::kNone;
    std::uint64_t id = 0;
  };

  struct avatar_info {
    glm::uvec2 dimensions{0};
    std::span<const std::uint8_t> rgba_buffer;
  };

  struct user_info {
    std::uint64_t id = 0;
    ustring name = ustring::ascii("");
  };

  struct friend_info {
    user_info user;
    bool in_game = false;
    std::optional<std::uint64_t> lobby_id;
  };

  struct lobby_info {
    bool is_host = false;
    std::uint64_t id = 0;
    std::uint32_t players = 0;
    std::uint32_t max_players = 0;
    std::vector<user_info> members;
  };

  virtual ~System() = default;
  virtual result<std::vector<std::string>> init() = 0;

  virtual void tick() = 0;
  virtual const std::vector<event>& events() const = 0;

  virtual bool supports_networked_multiplayer() const = 0;
  virtual user_info local_user() const = 0;
  virtual const std::vector<friend_info>& friend_list() const = 0;
  virtual std::optional<avatar_info> avatar(std::uint64_t user_id) const = 0;

  virtual void leave_lobby() = 0;
  virtual async_result<void> create_lobby() = 0;
  virtual async_result<void> join_lobby(std::uint64_t lobby_id) = 0;
  virtual std::optional<lobby_info> current_lobby() const = 0;
};

std::unique_ptr<System> create_system();

}  // namespace ii

#endif