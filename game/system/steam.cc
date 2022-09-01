#include "game/system/steam.h"
#include <steam/steam_api.h>
#include <iostream>

namespace ii {
namespace {
constexpr std::uint32_t kSteamAppId = 2139740u;
}  // namespace

SteamSystem::~SteamSystem() {
  SteamAPI_Shutdown();
}

result<std::vector<std::string>> SteamSystem::init(int, const char**) {
  if (SteamAPI_RestartAppIfNecessary(kSteamAppId)) {
    return unexpected("relaunching through steam");
  }
  if (!SteamAPI_Init()) {
    return unexpected("ERROR: failed to initialize steam API");
  }
  static constexpr std::size_t kBufferStartSize = 1024;
  std::string s;
  s.resize(kBufferStartSize);
  while (SteamApps()->GetLaunchCommandLine(s.data(), static_cast<int>(s.size())) == s.size()) {
    s.resize(s.size() * 2);
  }
  std::cerr << "Launch command-line:" << s << std::endl;
  std::vector<std::string> v;
  v.emplace_back(s);
  return v;
}

}  // namespace ii