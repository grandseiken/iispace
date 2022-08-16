#ifndef II_GAME_DATA_REPLAY_H
#define II_GAME_DATA_REPLAY_H
#include "game/common/result.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace ii::io {
class Filesystem;
}  // namespace ii::io

namespace ii {

class ReplayReader {
public:
  ~ReplayReader();
  ReplayReader(ReplayReader&&) noexcept;
  ReplayReader& operator=(ReplayReader&&) noexcept;

  static result<ReplayReader> create(std::span<const std::uint8_t> bytes);
  ii::initial_conditions initial_conditions() const;
  std::optional<input_frame> next_input_frame();
  std::vector<input_frame> next_tick_input_frames();

  std::size_t current_input_frame() const;
  std::size_t total_input_frames() const;

private:
  ReplayReader();
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

class ReplayWriter {
public:
  ~ReplayWriter();
  ReplayWriter(ReplayWriter&&) noexcept;
  ReplayWriter& operator=(ReplayWriter&&) noexcept;

  ReplayWriter(const ii::initial_conditions& conditions);
  void add_input_frame(const input_frame& frame);
  result<std::vector<std::uint8_t>> write() const;
  const ii::initial_conditions& initial_conditions() const;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif
