#include "game/data/crypt.h"
#include "game/common/raw_ptr.h"
#include <zlib.h>

namespace ii {

std::vector<std::uint8_t>
crypt(std::span<const std::uint8_t> text, std::span<const std::uint8_t> key) {
  std::vector<std::uint8_t> result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto b = text[i] ^ key[i % key.size()];
    if (!text[i] || !b) {
      result.emplace_back(text[i]);
    } else {
      result.emplace_back(b);
    }
  }
  return result;
}

ii::result<std::vector<std::uint8_t>> compress(std::span<const std::uint8_t> bytes) {
  static constexpr std::int32_t kCompressionLevel = Z_BEST_COMPRESSION;
  z_stream zs = {0};
  if (deflateInit(&zs, kCompressionLevel) != Z_OK) {
    return unexpected("deflateInit failed while compressing.");
  }
  auto zs_unique = ii::make_raw(
      &zs, +[](z_stream* s) { deflateEnd(s); });

  zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(bytes.data()));
  zs.avail_in = static_cast<uInt>(bytes.size());

  std::int32_t result = 0;
  std::uint8_t out_buffer[32768];
  std::vector<std::uint8_t> compressed;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
    zs.avail_out = sizeof(out_buffer);
    result = deflate(&zs, Z_FINISH);

    if (compressed.size() < zs.total_out) {
      auto size = zs.total_out - compressed.size();
      compressed.insert(compressed.end(), out_buffer, out_buffer + size);
    }
  } while (result == Z_OK);

  if (result != Z_STREAM_END) {
    return unexpected("compression error " + std::to_string(result) + ": " + std::string{zs.msg});
  }
  return {std::move(compressed)};
}

ii::result<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> bytes) {
  z_stream zs = {0};
  if (inflateInit(&zs) != Z_OK) {
    return unexpected("inflateInit failed while compressing.");
  }
  auto zs_unique = ii::make_raw(
      &zs, +[](z_stream* s) { inflateEnd(s); });

  zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(bytes.data()));
  zs.avail_in = static_cast<uInt>(bytes.size());

  std::int32_t result = 0;
  std::uint8_t out_buffer[32768];
  std::vector<std::uint8_t> decompressed;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
    zs.avail_out = sizeof(out_buffer);
    result = inflate(&zs, 0);

    if (decompressed.size() < zs.total_out) {
      auto size = zs.total_out - decompressed.size();
      decompressed.insert(decompressed.end(), out_buffer, out_buffer + size);
    }
  } while (result == Z_OK);

  if (result != Z_STREAM_END) {
    return unexpected("compression error " + std::to_string(result) + ": " + std::string{zs.msg});
  }
  return {std::move(decompressed)};
}

}  // namespace ii