#include "game/mixer/mixer.h"
#include "game/common/raw_ptr.h"
#include <dr_wav.h>
#include <samplerate.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace ii {
namespace {
constexpr float kPiFloat = 3.14159265358979323846264338327f;

void drwav_free(drwav* wav) {
  drwav_uninit(wav);
}

result<Mixer::audio_clip> drwav_read(drwav& wav) {
  Mixer::audio_clip result;
  result.sample_rate_hz = wav.sampleRate;
  auto frames = wav.totalPCMFrameCount;

  std::vector<float> data;
  data.resize(frames * wav.channels);
  if (drwav_read_pcm_frames_f32(&wav, frames, data.data()) != frames) {
    return unexpected("Invalid wav data");
  }

  if (wav.channels == 1) {
    result.samples = std::move(data);
    return {std::move(result)};
  }

  result.samples.resize(frames);
  for (std::size_t i = 0; i < frames; ++i) {
    float f = 0.f;
    for (std::uint16_t k = 0; k < wav.channels; ++k) {
      f += data[k + i * wav.channels];
    }
    result.samples[i] = f / wav.channels;
  }
  return {std::move(result)};
}

result<Mixer::audio_clip> drwav_load_memory(nonstd::span<std::uint8_t> data) {
  drwav wav;
  if (!drwav_init_memory(&wav, data.data(), data.size(), /* allocation */ nullptr)) {
    return unexpected("Couldn't read in-memory wav");
  }
  auto wav_handle = make_raw(&wav, &drwav_free);
  return drwav_read(wav);
}

result<std::vector<float>>
resample_mono(const std::vector<float>& samples, double target_source_ratio) {
  std::vector<float> output;
  output.resize(static_cast<std::size_t>(std::ceil(1 + target_source_ratio * samples.size())));

  SRC_DATA src_data = {0};
  src_data.input_frames = static_cast<long>(samples.size());
  src_data.output_frames = static_cast<long>(output.size());
  src_data.data_in = samples.data();
  src_data.data_out = output.data();
  src_data.src_ratio = target_source_ratio;
  auto error = src_simple(&src_data, SRC_SINC_FASTEST, /* channels */ 1);
  if (error) {
    return unexpected(src_strerror(error));
  }
  output.resize(static_cast<std::size_t>(src_data.output_frames_gen));
  return {std::move(output)};
}

Mixer::audio_sample_t f2s16(float v) {
  return static_cast<Mixer::audio_sample_t>(
      std::floor(.5f + v * std::numeric_limits<Mixer::audio_sample_t>::max()));
}

}  // namespace

Mixer::Mixer(std::uint32_t sample_rate_hz) : sample_rate_hz_{sample_rate_hz} {}

void Mixer::set_master_volume(float volume) {
  master_volume_ = std::clamp(volume, 0.f, 1.f);
}

result<Mixer::audio_handle_t>
Mixer::load_wav_memory(nonstd::span<std::uint8_t> data, std::optional<audio_handle_t> handle) {
  auto h = assign_handle(handle);
  if (!h) {
    return unexpected(h.error());
  }
  auto clip = drwav_load_memory(data);
  if (!clip) {
    return unexpected(clip.error());
  }
  audio_resource r;
  r.clip = std::move(*clip);
  audio_resources_.emplace(*h, std::move(r));
  return *h;
}

void Mixer::release_handle(audio_handle_t) {
  // TODO
}

void Mixer::play(audio_handle_t handle, float volume, float pan, float pitch) {
  auto it = audio_resources_.find(handle);
  if (it == audio_resources_.end() || !it->second.clip) {
    return;
  }
  const auto& clip = *it->second.clip;

  volume = std::clamp(volume, 0.f, 1.f);
  pan = std::clamp(kPiFloat / 4.f * (pan + 1.f), 0.f, kPiFloat / 2.f);
  pitch = std::clamp(pitch, 1.f / 64, 64.f);

  auto ratio = static_cast<double>(sample_rate_hz_) / clip.sample_rate_hz;
  ratio /= pitch;
  // TODO: probably shouldn't resample all at once on the fly. Expensive?
  auto resampled = resample_mono(clip.samples, ratio);
  if (!resampled) {
    return;
  }

  sound s;
  s.lvolume = std::clamp(volume * std::cos(pan), 0.f, 1.f);
  s.rvolume = std::clamp(volume * std::sin(pan), 0.f, 1.f);
  s.samples = std::move(*resampled);
  new_sounds_.emplace_back(std::move(s));
}

void Mixer::commit() {
  std::unique_lock lock{sound_mutex_};
  sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
                               [](const sound& s) { return s.position >= s.samples.size(); }),
                sounds_.end());
  for (auto& s : new_sounds_) {
    sounds_.emplace_back(std::move(s));
  }
  new_sounds_.clear();
}

void Mixer::audio_callback(std::uint8_t* out_buffer, std::size_t samples) {
  mix_buffer_.resize(2 * samples);
  std::fill(mix_buffer_.begin(), mix_buffer_.end(), 0.f);
  auto master_volume = master_volume_.load();
  {
    std::unique_lock lock{sound_mutex_};
    for (auto& s : sounds_) {
      auto sample_count = std::min(samples, s.samples.size() - s.position);
      for (std::size_t i = 0; i < sample_count; ++i) {
        mix_buffer_[2 * i] += s.lvolume * s.samples[s.position + i];
        mix_buffer_[2 * i + 1] += s.rvolume * s.samples[s.position + i];
      }
      s.position += sample_count;
    }
  }
  for (auto& s : mix_buffer_) {
    s *= master_volume;
  }
  for (std::size_t i = 0; i < samples; ++i) {
    auto v = std::max(std::abs(mix_buffer_[2 * i]), std::abs(mix_buffer_[2 * i + 1]));
    auto m = std::tanh(v) / v;
    auto l = f2s16(m * mix_buffer_[2 * i]);
    auto r = f2s16(m * mix_buffer_[2 * i + 1]);
    std::memcpy(out_buffer + 2 * i * sizeof(audio_sample_t), &l, sizeof(audio_sample_t));
    std::memcpy(out_buffer + (2 * i + 1) * sizeof(audio_sample_t), &r, sizeof(audio_sample_t));
  }
}

result<Mixer::audio_handle_t> Mixer::assign_handle(std::optional<audio_handle_t> requested) {
  auto handle = requested ? *requested : next_handle_++;
  if (audio_resources_.find(handle) != audio_resources_.end()) {
    return unexpected("Audio handle already in use");
  }
  return handle;
}

}  // namespace ii