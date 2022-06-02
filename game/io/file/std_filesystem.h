#ifndef II_GAME_IO_FILE_STD_FILESYSTEM_H
#define II_GAME_IO_FILE_STD_FILESYSTEM_H
#include "game/io/file/filesystem.h"
#include <string>

namespace ii::io {

class StdFilesystem : public Filesystem {
public:
  StdFilesystem(std::string_view asset_dir, std::string_view save_dir, std::string_view replay_dir);
  ~StdFilesystem() override {}

  result<byte_buffer> read(std::string_view name) const override;

  result<byte_buffer> read_asset(std::string_view name) const override;
  result<byte_buffer> read_config() const override;
  result<void> write_config(nonstd::span<const std::uint8_t>) override;

  std::vector<std::string> list_savegames() const override;
  result<byte_buffer> read_savegame(std::string_view name) const override;
  result<void> write_savegame(std::string_view name, nonstd::span<const std::uint8_t>) override;

  std::vector<std::string> list_replays() const override;
  result<byte_buffer> read_replay(std::string_view name) const override;
  result<void> write_replay(std::string_view name, nonstd::span<const std::uint8_t>) override;

private:
  std::string asset_dir_;
  std::string save_dir_;
  std::string replay_dir_;
};

}  // namespace ii::io

#endif