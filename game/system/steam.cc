#include "game/system/steam.h"
#include <steam/steam_api.h>

namespace ii {
namespace {
constexpr std::uint64_t kSteamAppId = 2139740u;
constexpr std::uint32_t kLobbyMaxMembers = 4u;

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
    info.id = friend_id.ConvertToUint64();
    info.username = ustring::utf8(SteamFriends()->GetFriendPersonaName(friend_id));

    FriendGameInfo_t game_info;
    if (SteamFriends()->GetFriendGamePlayed(friend_id, &game_info)) {
      if (game_info.m_gameID.ToUint64() == kSteamAppId) {
        info.in_game = true;
        if (game_info.m_steamIDLobby.IsLobby() && game_info.m_steamIDLobby.IsValid()) {
          info.lobby_id = game_info.m_steamIDLobby.ConvertToUint64();
        }
      }
    }
  }
  return result;
}

template <typename T>
class CallResult {
private:
  struct access_tag {};

public:
  static void on_complete(SteamAPICall_t call_id, System::callback<const T*> callback) {
    auto p = std::make_unique<CallResult>(access_tag{}, call_id, std::move(callback));
    p->self_ = std::move(p);
  }

  CallResult(access_tag, SteamAPICall_t call_id, System::callback<const T*>&& callback)
  : callback_{std::move(callback)} {
    call_result_.Set(call_id, this, &CallResult::on_complete_internal);
  }

private:
  void on_complete_internal(const T* data, bool io_failure) {
    auto scoped_destroy = std::move(self_);
    if (io_failure) {
      callback_(unexpected("steam API call failed"));
    } else {
      callback_(data);
    }
  }

  std::unique_ptr<CallResult> self_;
  System::callback<const T*> callback_;
  CCallResult<CallResult, const T> call_result_;
};

}  // namespace

struct SteamSystem::impl_t {
  ~impl_t() { SteamAPI_Shutdown(); }
  impl_t()
  : game_overlay_activated_cb{this, &impl_t::on_game_overlay_activated}
  , persona_state_change_cb{this, &impl_t::on_persona_state_change} {}

  template <typename T>
  using callback_t = CCallback<impl_t, const T>;
  callback_t<GameOverlayActivated_t> game_overlay_activated_cb;
  callback_t<PersonaStateChange_t> persona_state_change_cb;

  void on_game_overlay_activated(const GameOverlayActivated_t*) {
    events.emplace_back().type = event_type::kOverlayActivated;
  }

  void on_persona_state_change(const PersonaStateChange_t*) { friend_list.reset(); }

  std::vector<event> events;
  std::optional<std::vector<friend_info>> friend_list;
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

ustring SteamSystem::local_username() const {
  return ustring::utf8(SteamFriends()->GetPersonaName());
}

auto SteamSystem::friend_list() const -> const std::vector<friend_info>& {
  if (!impl_->friend_list) {
    impl_->friend_list = get_steam_friend_list();
  }
  return *impl_->friend_list;
}

void SteamSystem::create_lobby(callback<void> cb) {
  auto call_id =
      SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, static_cast<int>(kLobbyMaxMembers));
  CallResult<LobbyCreated_t>::on_complete(call_id,
                                          [cb = std::move(cb)](result<const LobbyCreated_t*> data) {
                                            if (!data) {
                                              cb(unexpected(data.error()));
                                            } else if ((*data)->m_eResult != k_EResultOK) {
                                              cb(unexpected("failed to create steam lobby"));
                                            } else {
                                              cb({});
                                            }
                                          });
}

}  // namespace ii