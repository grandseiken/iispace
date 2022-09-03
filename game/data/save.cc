#include "game/data/save.h"
#include "game/data/crypt.h"
#include "game/data/proto/savegame.pb.h"
#include <algorithm>
#include <array>
#include <sstream>

namespace ii::data {
namespace {
const std::array<std::uint8_t, 2> kSaveEncryptionKey = {'<', '>'};
}  // namespace

result<savegame> read_savegame(std::span<const std::uint8_t> bytes) {
  proto::SaveGame proto;
  auto d = crypt(bytes, kSaveEncryptionKey);
  if (!proto.ParseFromArray(d.data(), static_cast<int>(d.size()))) {
    return unexpected("couldn't parse savegame");
  }

  savegame data;
  data.bosses_killed = boss_flag{static_cast<std::uint32_t>(proto.bosses_killed())};
  data.hard_mode_bosses_killed =
      boss_flag{static_cast<std::uint32_t>(proto.hard_mode_bosses_killed())};
  return {data};
}

result<std::vector<std::uint8_t>> write_savegame(const savegame& data) {
  proto::SaveGame proto;
  proto.set_bosses_killed(+data.bosses_killed);
  proto.set_hard_mode_bosses_killed(+data.hard_mode_bosses_killed);

  std::vector<std::uint8_t> bytes;
  bytes.resize(proto.ByteSizeLong());
  if (!proto.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()))) {
    return unexpected("couldn't serialize savegame");
  }
  return crypt(bytes, kSaveEncryptionKey);
}

}  // namespace ii::data
