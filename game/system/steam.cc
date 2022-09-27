#include "game/system/steam.h"
#include <sfn/functional.h>
#include <steam/steam_api.h>
#include <functional>
#include <type_traits>
#include <unordered_map>

namespace ii {
namespace {
constexpr std::uint64_t kSteamAppId = 2139740u;
constexpr std::uint32_t kLobbyMaxMembers = 8u;

std::vector<std::string> get_steam_command_line() {
  static constexpr std::size_t kBufferStartSize = 1024;
  std::string s;
  s.resize(kBufferStartSize);
  int length = 0;
  while ((length = SteamApps()->GetLaunchCommandLine(s.data(), static_cast<int>(s.size()))) ==
         s.size()) {
    s.resize(s.size() * 2);
  }
  s.resize(length);
  std::vector<std::string> v;
  std::string current_arg;
  bool in_quote = false;
  for (char c : s) {
    if (!c) {
      break;
    }
    if (c == '"') {
      in_quote = !in_quote;
      continue;
    }
    if (!in_quote && (c == ' ' || c == '\n' || c == '\t' || c == '\r')) {
      if (!current_arg.empty()) {
        v.emplace_back(current_arg);
        current_arg.clear();
      }
      continue;
    }
    current_arg += c;
  }
  if (!current_arg.empty()) {
    v.emplace_back(current_arg);
  }
  return v;
}

std::vector<System::friend_info> get_steam_friend_list() {
  std::vector<System::friend_info> result;
  auto friend_count = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
  if (friend_count < 0) {
    return result;
  }
  for (int i = 0; i < friend_count; ++i) {
    auto friend_id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
    if (SteamFriends()->GetFriendPersonaState(friend_id) == k_EPersonaStateOffline) {
      continue;
    }
    auto& info = result.emplace_back();
    info.user.id = friend_id.ConvertToUint64();
    info.user.name = ustring::utf8(SteamFriends()->GetFriendPersonaName(friend_id));

    FriendGameInfo_t game_info;
    if (SteamFriends()->GetFriendGamePlayed(friend_id, &game_info) &&
        game_info.m_gameID.ToUint64() == kSteamAppId) {
      info.in_game = true;
      if (game_info.m_steamIDLobby.IsValid()) {
        info.lobby_id = game_info.m_steamIDLobby.ConvertToUint64();
      }
    }
  }
  return result;
}

System::lobby_info get_lobby_info(std::uint64_t lobby_id) {
  System::lobby_info info;
  info.id = lobby_id;
  info.is_host = SteamMatchmaking()->GetLobbyOwner(lobby_id) == SteamUser()->GetSteamID();
  info.max_players = static_cast<std::uint32_t>(SteamMatchmaking()->GetLobbyMemberLimit(lobby_id));

  auto num_players = SteamMatchmaking()->GetNumLobbyMembers(lobby_id);
  info.players = static_cast<std::uint32_t>(num_players);
  for (int i = 0; i < num_players; ++i) {
    auto id = SteamMatchmaking()->GetLobbyMemberByIndex(lobby_id, i);
    auto& m = info.members.emplace_back();
    m.id = id.ConvertToUint64();
    m.name = ustring::utf8(SteamFriends()->GetFriendPersonaName(id));
  }
  return info;
}

template <typename T, typename R = void>
class CallResult {
private:
  struct access_tag {};

public:
  using callback_t = std::function<result<R>(const T*)>;

  static void on_complete(SteamAPICall_t call_id, promise_result<R> promise, callback_t callback) {
    auto p = std::make_unique<CallResult>(access_tag{}, call_id, std::move(promise),
                                          std::move(callback));
    p->self_ = std::move(p);
  }

  CallResult(access_tag, SteamAPICall_t call_id, promise_result<R> promise, callback_t&& callback)
  : promise_{std::move(promise)}, callback_{std::move(callback)} {
    call_result_.Set(call_id, this, &CallResult::on_complete_internal);
  }

private:
  void on_complete_internal(const T* data, bool io_failure) {
    auto scoped_destroy = std::move(self_);
    if (io_failure) {
      promise_.set(unexpected("Steam API call failed"));
    } else {
      promise_.set(callback_(data));
    }
  }

  std::unique_ptr<CallResult> self_;
  promise_result<R> promise_;
  callback_t callback_;
  CCallResult<CallResult, const T> call_result_;
};

}  // namespace

struct SteamSystem::impl_t {
  ~impl_t() { SteamAPI_Shutdown(); }
  impl_t() = default;

  void leave_lobby() {
    if (current_lobby) {
      SteamMatchmaking()->LeaveLobby(current_lobby->id);
      current_lobby.reset();
    }
  }

  void set_lobby(std::uint64_t lobby_id) {
    if (current_lobby && current_lobby->id != lobby_id) {
      SteamMatchmaking()->LeaveLobby(current_lobby->id);
    }
    current_lobby = get_lobby_info(lobby_id);
  }

  template <sfn::member_function auto F>
  struct callback {
    using arg_type = sfn::back<sfn::parameter_types_of<decltype(F)>>;
    using type = std::remove_pointer_t<arg_type>;
    CCallback<impl_t, type> cb;
    callback(impl_t* p) : cb{p, F} {}
  };

  void persona_state_change(const PersonaStateChange_t*) { friend_list.reset(); }
  void game_overlay_activated(const GameOverlayActivated_t*) {
    events.emplace_back().type = event_type::kOverlayActivated;
  }

  void game_lobby_join_requested(const GameLobbyJoinRequested_t* data) {
    // TODO: handle this from command-line also.
    auto& e = events.emplace_back();
    e.type = event_type::kLobbyJoinRequested;
    e.id = data->m_steamIDLobby.ConvertToUint64();
  }

  void lobby_enter(const LobbyEnter_t* data) {
    // This flow triggered from create_lobby().
    if (data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
      set_lobby(data->m_ulSteamIDLobby);
    }
  }

  void avatar_image_loaded(const AvatarImageLoaded_t* data) {
    auto& avatar = avatars[data->m_steamID.ConvertToUint64()];
    if (SteamUtils()->GetImageSize(data->m_iImage, &avatar.dimensions.x, &avatar.dimensions.y)) {
      avatar.rgba_buffer.resize(4 * avatar.dimensions.x * avatar.dimensions.y);
      SteamUtils()->GetImageRGBA(data->m_iImage, avatar.rgba_buffer.data(),
                                 static_cast<int>(avatar.rgba_buffer.size()));
    }
  }

  callback<&impl_t::persona_state_change> persona_state_change_cb{this};
  callback<&impl_t::game_overlay_activated> game_overlay_activated_cb{this};
  callback<&impl_t::game_lobby_join_requested> game_lobby_join_requested_cb{this};
  callback<&impl_t::lobby_enter> lobby_enter_cb{this};

  struct avatar_data {
    glm::uvec2 dimensions{0, 0};
    std::vector<std::uint8_t> rgba_buffer;
  };

  std::vector<event> events;
  std::optional<std::vector<friend_info>> friend_list;
  std::optional<lobby_info> current_lobby;
  std::unordered_map<std::uint64_t, avatar_data> avatars;
};

void OnGameOverlayActivated(GameOverlayActivated_t*) {}

SteamSystem::SteamSystem() = default;
SteamSystem::~SteamSystem() = default;

result<std::vector<std::string>> SteamSystem::init() {
  if (SteamAPI_RestartAppIfNecessary(kSteamAppId)) {
    return unexpected("relaunching through steam");
  }
  if (!SteamAPI_Init()) {
    return unexpected("ERROR: failed to initialize steam API");
  }
  impl_ = std::make_unique<impl_t>();
  return get_steam_command_line();
}

void SteamSystem::tick() {
  impl_->events.clear();
  SteamAPI_RunCallbacks();
}

auto SteamSystem::events() const -> const std::vector<event>& {
  return impl_->events;
}

bool SteamSystem::supports_networked_multiplayer() const {
  return true;
}

auto SteamSystem::local_user() const -> user_info {
  user_info user;
  user.id = SteamUser()->GetSteamID().ConvertToUint64();
  user.name = ustring::utf8(SteamFriends()->GetPersonaName());
  return user;
}

auto SteamSystem::friend_list() const -> const std::vector<friend_info>& {
  if (!impl_->friend_list) {
    impl_->friend_list = get_steam_friend_list();
  }
  return *impl_->friend_list;
}

auto SteamSystem::avatar(std::uint64_t user_id) const -> std::optional<avatar_info> {
  if (!impl_->avatars.contains(user_id)) {
    SteamFriends()->GetLargeFriendAvatar(user_id);
    return std::nullopt;
  }
  auto& data = impl_->avatars[user_id];
  if (!data.dimensions.x || !data.dimensions.y) {
    return std::nullopt;  // Not loaded yet.
  }

  avatar_info info;
  info.dimensions = data.dimensions;
  info.rgba_buffer = data.rgba_buffer;
  return info;
}

void SteamSystem::leave_lobby() {
  impl_->leave_lobby();
}

async_result<void> SteamSystem::create_lobby() {
  auto call_id =
      SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, static_cast<int>(kLobbyMaxMembers));

  promise_result<void> promise;
  auto future = promise.future();
  CallResult<LobbyCreated_t>::on_complete(
      call_id, std::move(promise), [](const LobbyCreated_t* data) -> result<void> {
        if (data->m_eResult != k_EResultOK) {
          return unexpected("Couldn't create steam lobby: " + std::to_string(data->m_eResult));
        }
        return {};
      });
  return future;
}

async_result<void> SteamSystem::join_lobby(std::uint64_t lobby_id) {
  auto call_id = SteamMatchmaking()->JoinLobby(lobby_id);

  promise_result<void> promise;
  auto future = promise.future();
  CallResult<LobbyEnter_t>::on_complete(
      call_id, std::move(promise), [this](const LobbyEnter_t* data) -> result<void> {
        if (data->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess) {
          return unexpected("Couldn't join steam lobby: " +
                            std::to_string(data->m_EChatRoomEnterResponse));
        }
        impl_->set_lobby(data->m_ulSteamIDLobby);
        return {};
      });
  return future;
}

auto SteamSystem::current_lobby() const -> std::optional<lobby_info> {
  return impl_->current_lobby;
}

}  // namespace ii