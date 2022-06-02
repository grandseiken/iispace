#ifndef II_GAME_IO_FILE_FILESYSTEM_H
#define II_GAME_IO_FILE_FILESYSTEM_H
#include "game/common/result.h"
#include <nonstd/span.hpp>
#include <cstdint>
#include <string_view>
#include <vector>

namespace ii::io {

class Filesystem {
public:
  using byte_buffer = std::vector<std::uint8_t>;
  virtual ~Filesystem() {}

  virtual result<byte_buffer> read(std::string_view name) const = 0;

  virtual result<byte_buffer> read_asset(std::string_view name) const = 0;
  virtual result<byte_buffer> read_config() const = 0;
  virtual result<void> write_config(nonstd::span<const std::uint8_t>) = 0;

  virtual std::vector<std::string> list_savegames() const = 0;
  virtual result<byte_buffer> read_savegame(std::string_view name) const = 0;
  virtual result<void> write_savegame(std::string_view name, nonstd::span<const std::uint8_t>) = 0;

  virtual std::vector<std::string> list_replays() const = 0;
  virtual result<byte_buffer> read_replay(std::string_view name) const = 0;
  virtual result<void> write_replay(std::string_view name, nonstd::span<const std::uint8_t>) = 0;
};

}  // namespace ii::io

#endif