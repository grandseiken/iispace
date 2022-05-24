#ifndef IISPACE_GAME_MIXER_MIXER_H
#define IISPACE_GAME_MIXER_MIXER_H
#include "game/common/result.h"
#include <nonstd/span.hpp>
#include <cstddef>
#include <cstdint>

namespace ii {

class Mixer {
public:
  static constexpr std::uint32_t kAudioSampleRate = 48000;
  using audio_sample_t = std::int16_t;
  using audio_handle_t = std::size_t;

  Mixer();
  result<audio_handle_t> load_wav_file(const std::string& filename);
  result<audio_handle_t> load_wav_memory(nonstd::span<std::uint8_t> data);
  void release_handle(audio_handle_t);

  void set_master_volume(float volume);
  void play_sound(audio_handle_t, float volume, float pan, float pitch);
  void audio_callback(std::uint8_t* out_buffer, std::size_t samples);

private:
  float master_volume_ = 1.f;
};

}  // namespace ii

#endif