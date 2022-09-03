#include "game/system/steam.h"
#include <steam/steam_api.h>

namespace ii {
namespace {
constexpr std::uint32_t kSteamAppId = 2139740u;

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
}  // namespace

SteamSystem::~SteamSystem() {
  SteamAPI_Shutdown();
}

result<std::vector<std::string>> SteamSystem::init() {
  if (SteamAPI_RestartAppIfNecessary(kSteamAppId)) {
    return unexpected("relaunching through steam");
  }
  if (!SteamAPI_Init()) {
    return unexpected("ERROR: failed to initialize steam API");
  }
  return get_steam_command_line();
}

}  // namespace ii