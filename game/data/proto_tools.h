#ifndef II_GAME_DATA_PROTO_TOOLS_H
#define II_GAME_DATA_PROTO_TOOLS_H
#include "game/common/result.h"
#include <cstdint>
#include <span>

namespace ii::data {

template <typename T>
result<T> read_proto(std::span<const std::uint8_t> bytes) {
  T proto;
  if (!proto.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    return unexpected("invalid data");
  }
  return {std::move(proto)};
}

template <typename T>
result<std::vector<std::uint8_t>> write_proto(const T& proto) {
  std::vector<std::uint8_t> bytes;
  bytes.resize(proto.ByteSizeLong());
  if (!proto.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()))) {
    return unexpected("couldn't serialize data");
  }
  return {std::move(bytes)};
}

}  // namespace ii::data

#endif
