#include "game/io/file/std_filesystem.h"
#include <filesystem>
#include <fstream>

namespace ii::io {
namespace {
const char* kConfigPath = "config.dat";
const char* kSaveExt = ".sav";
const char* kReplayExt = ".wrp";

result<std::vector<std::uint8_t>> read(const std::filesystem::path& path) {
  std::ifstream f{path, std::ios::in | std::ios::binary};
  if (!f.is_open()) {
    return unexpected("Couldn't open " + path.string() + " for reading");
  }
  f.unsetf(std::ios::skipws);

  f.seekg(0, std::ios::end);
  auto size = f.tellg();
  f.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> v;
  v.reserve(size);
  v.insert(v.begin(), std::istream_iterator<std::uint8_t>(f),
           std::istream_iterator<std::uint8_t>());
  return {std::move(v)};
}

result<void> write(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
  std::ofstream f{path, std::ios::out | std::ios::binary};
  if (!f.is_open()) {
    return unexpected("Couldn't open " + path.string() + " for writing");
  }
  f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!f.good()) {
    return unexpected("Error writing " + path.string());
  }
  return {};
}

}  // namespace

StdFilesystem::StdFilesystem(std::string_view asset_dir, std::string_view save_dir,
                             std::string_view replay_dir)
: asset_dir_{asset_dir}, save_dir_{save_dir}, replay_dir_{replay_dir} {
  std::error_code ec;
  std::filesystem::create_directories(save_dir_, ec);
  std::filesystem::create_directories(replay_dir_, ec);
}

result<Filesystem::byte_buffer> StdFilesystem::read(std::string_view name) const {
  return io::read(std::filesystem::path{std::string{name}});
}

result<Filesystem::byte_buffer> StdFilesystem::read_asset(std::string_view name) const {
  return io::read(std::filesystem::path{asset_dir_} / name);
}

result<Filesystem::byte_buffer> StdFilesystem::read_config() const {
  return io::read(std::filesystem::path{save_dir_} / kConfigPath);
}

result<void> StdFilesystem::write_config(std::span<const std::uint8_t> data) {
  return io::write(std::filesystem::path{save_dir_} / kConfigPath, data);
}

std::vector<std::string> StdFilesystem::list_savegames() const {
  std::vector<std::string> result;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator{save_dir_, ec}) {
    if (entry.is_regular_file() && entry.path().extension() == kSaveExt) {
      result.emplace_back(entry.path().stem().string());
    }
  }
  return {std::move(result)};
}

result<Filesystem::byte_buffer> StdFilesystem::read_savegame(std::string_view name) const {
  return io::read(std::filesystem::path{save_dir_} / (std::string{name} + kSaveExt));
}

result<void>
StdFilesystem::write_savegame(std::string_view name, std::span<const std::uint8_t> data) {
  return io::write(std::filesystem::path{save_dir_} / (std::string{name} + kSaveExt), data);
}

std::vector<std::string> StdFilesystem::list_replays() const {
  std::vector<std::string> result;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator{replay_dir_, ec}) {
    if (entry.is_regular_file() && entry.path().extension() == kReplayExt) {
      result.emplace_back(entry.path().stem().string());
    }
  }
  return {std::move(result)};
}

result<Filesystem::byte_buffer> StdFilesystem::read_replay(std::string_view name) const {
  return io::read(std::filesystem::path{replay_dir_} / (std::string{name} + kReplayExt));
}

result<void>
StdFilesystem::write_replay(std::string_view name, std::span<const std::uint8_t> data) {
  return io::write(std::filesystem::path{replay_dir_} / (std::string{name} + kReplayExt), data);
}

}  // namespace ii::io