#include "game/data/config.h"
#include "game/data/proto/config.pb.h"
#include "game/data/proto_tools.h"

namespace ii::data {

result<config> read_config(std::span<const std::uint8_t> bytes) {
  auto proto = read_proto<proto::Config>(bytes);
  if (!proto) {
    return unexpected(proto.error());
  }

  config c;
  c.volume = proto->volume();
  return {c};
}

result<std::vector<std::uint8_t>> write_config(const config& c) {
  proto::Config proto;
  proto.set_volume(c.volume);
  return write_proto(proto);
}

}  // namespace ii::data
