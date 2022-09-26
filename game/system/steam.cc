#include "game/system/steam.h"
#include <sfn/functional.h>
#include <steam/steam_api.h>
#include <functional>
#include <type_traits>

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
    if (data->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
      current_lobby = get_lobby_info(data->m_ulSteamIDLobby);
    }
  }

  callback<&impl_t::persona_state_change> persona_state_change_cb{this};
  callback<&impl_t::game_overlay_activated> game_overlay_activated_cb{this};
  callback<&impl_t::game_lobby_join_requested> game_lobby_join_requested_cb{this};
  callback<&impl_t::lobby_enter> lobby_enter_cb{this};

  std::vector<event> events;
  std::optional<std::vector<friend_info>> friend_list;
  std::optional<lobby_info> current_lobby;
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

void SteamSystem::leave_lobby() {
  if (impl_->current_lobby) {
    SteamMatchmaking()->LeaveLobby(impl_->current_lobby->id);
    impl_->current_lobby.reset();
  }
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
  return {unexpected("NYI")};  // TODO
}

auto SteamSystem::current_lobby() const -> std::optional<lobby_info> {
  return std::nullopt;  // TODO
}

auto SteamSystem::lobby_results() const -> const std::vector<lobby_info>& {
  static const std::vector<lobby_info> kEmpty;
  return kEmpty;  // TODO
}

}  // namespace ii