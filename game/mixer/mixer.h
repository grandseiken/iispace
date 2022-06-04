#ifndef II_GAME_MIXER_MIXER_H
#define II_GAME_MIXER_MIXER_H
#include "game/common/result.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace ii {

class Mixer {
public:
  using audio_sample_t = std::int16_t;
  using audio_handle_t = std::size_t;

  ~Mixer();
  Mixer(std::uint32_t sample_rate);
  result<audio_handle_t> load_wav_memory(std::span<std::uint8_t> data,
                                         std::optional<audio_handle_t> handle = std::nullopt);

  void set_master_volume(float volume);
  void play(audio_handle_t handle, float volume, float pan, float pitch);
  void commit();
  void audio_callback(std::uint8_t* out_buffer, std::size_t samples);

private:
  result<audio_handle_t> assign_handle(std::optional<audio_handle_t> requested);

  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif