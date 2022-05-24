#include "game/mixer/mixer.h"
#include <dr_wav.h>
#include <samplerate.h>

namespace ii {

Mixer::Mixer() = default;

void Mixer::set_master_volume(float volume) {
  master_volume_ = volume;
}

result<Mixer::audio_handle_t> Mixer::load_wav_file(const std::string& filename) {
  return unexpected("NYI");
}

result<Mixer::audio_handle_t> Mixer::load_wav_memory(nonstd::span<std::uint8_t> data) {
  return unexpected("NYI");
}

void Mixer::release_handle(audio_handle_t) {
  // TODO
}

void Mixer::play_sound(audio_handle_t, float volume, float pan, float pitch) {
  // TODO
}

void Mixer::audio_callback(std::uint8_t* out_buffer, std::size_t samples) {
  // TODO
}

}  // namespace ii