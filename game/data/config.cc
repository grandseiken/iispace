#include "game/data/config.h"
#include "game/data/proto/config.pb.h"

namespace ii::data {

result<config> read_config(std::span<const std::uint8_t> bytes) {
  proto::Config proto;
  if (!proto.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    return unexpected("couldn't parse settings");
  }

  config c;
  c.volume = proto.volume();
  return {c};
}

result<std::vector<std::uint8_t>> write_config(const config& c) {
  proto::Config proto;
  proto.set_volume(c.volume);
  std::vector<std::uint8_t> data;
  data.resize(proto.ByteSizeLong());
  if (!proto.SerializeToArray(data.data(), static_cast<int>(data.size()))) {
    return unexpected("couldn't serialize settings");
  }
  return {std::move(data)};
}

}  // namespace ii::data
