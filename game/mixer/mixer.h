#ifndef IISPACE_GAME_MIXER_MIXER_H
#define IISPACE_GAME_MIXER_MIXER_H
#include "game/common/result.h"
#include <nonstd/span.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {

class Mixer {
public:
  using audio_sample_t = std::int16_t;
  using audio_handle_t = std::size_t;

  Mixer(std::uint32_t sample_rate);
  result<audio_handle_t> load_wav_memory(nonstd::span<std::uint8_t> data,
                                         std::optional<audio_handle_t> handle = std::nullopt);
  void release_handle(audio_handle_t);

  void set_master_volume(float volume);
  void play(audio_handle_t handle, float volume, float pan, float pitch);
  void commit();
  void audio_callback(std::uint8_t* out_buffer, std::size_t samples);

  struct audio_clip {
    std::uint32_t sample_rate_hz = 0;
    std::vector<float> samples;
  };

private:
  result<audio_handle_t> assign_handle(std::optional<audio_handle_t> requested);

  // Audio resources.
  struct audio_resource {
    std::optional<audio_clip> clip;
  };
  audio_handle_t next_handle_ = 0;
  std::unordered_map<audio_handle_t, audio_resource> audio_resources_;

  // Playback.
  std::uint32_t sample_rate_hz_ = 0;
  std::atomic<float> master_volume_{1.f};

  struct sound {
    std::vector<float> samples;
    float lvolume = 0.f;
    float rvolume = 0.f;
    std::size_t position = 0;
  };
  std::vector<sound> new_sounds_;

  std::mutex sound_mutex_;
  std::vector<sound> sounds_;
  std::vector<float> mix_buffer_;
};

}  // namespace ii

#endif