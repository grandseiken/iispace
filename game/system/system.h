#ifndef II_GAME_SYSTEM_SYSTEM_H
#define II_GAME_SYSTEM_SYSTEM_H
#include "game/common/async.h"
#include "game/common/math.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
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
    // Messaging session with user failed.
    kMessagingSessionFailed,
  };

  struct event {
    event_type type = event_type::kNone;
    std::uint64_t id = 0;
  };

  struct avatar_info {
    uvec2 dimensions{0};
    std::span<const std::uint8_t> rgba_buffer;
  };

  struct user_info {
    std::uint64_t id = 0;
    ustring name;
  };

  struct friend_info {
    user_info user;
    bool in_game = false;
    std::optional<std::uint64_t> lobby_id;
  };

  struct lobby_info {
    std::uint64_t id = 0;
    std::uint32_t players = 0;
    std::uint32_t max_players = 0;
    // If empty, we are the host.
    std::optional<user_info> host;
    std::vector<user_info> members;
  };

  struct session_info {
    std::uint32_t ping_ms = 0;
    float quality = 0.f;
  };

  enum send_flags : std::uint32_t {
    kSendReliable = 1,
    kSendNoNagle = 2,
  };

  struct send_message {
    std::uint32_t send_flags = 0;
    std::uint32_t channel = 0;
    std::span<const std::uint8_t> bytes;
  };

  struct received_message {
    std::uint64_t source_user_id = 0;
    std::vector<std::uint8_t> bytes;
  };

  virtual ~System() = default;
  virtual result<std::vector<std::string>> init(int argc, const char** argv) = 0;

  virtual void tick() = 0;
  virtual const std::vector<event>& events() const = 0;

  // User API.
  virtual bool supports_networked_multiplayer() const = 0;
  virtual user_info local_user() const = 0;
  virtual const std::vector<friend_info>& friend_list() const = 0;
  virtual std::optional<avatar_info> avatar(std::uint64_t user_id) const = 0;

  // Lobby API.
  virtual void leave_lobby() = 0;
  virtual async_result<void> create_lobby() = 0;
  virtual async_result<void> join_lobby(std::uint64_t lobby_id) = 0;
  virtual const std::optional<lobby_info>& current_lobby() const = 0;
  virtual void show_invite_dialog() const = 0;

  // Messaging API.
  virtual std::optional<session_info> session(std::uint64_t user_id) const = 0;
  virtual void send_to(std::uint64_t user_id, const send_message&) = 0;
  virtual void send_to_host(const send_message&) = 0;
  virtual void broadcast(const send_message&) = 0;
  virtual void receive(std::uint32_t channel, std::vector<received_message>&) = 0;
};

std::unique_ptr<System> create_system();
std::optional<System::event> event_triggered(const System& system, System::event_type type);

}  // namespace ii

#endif