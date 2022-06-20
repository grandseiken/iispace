#include "game/mixer/mixer.h"
#include "game/common/raw_ptr.h"
#include <dr_wav.h>
#include <glm/gtc/constants.hpp>
#include <samplerate.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace ii {
namespace {

struct audio_clip {
  std::uint32_t sample_rate_hz = 0;
  std::vector<float> samples;
};

void src_free(SRC_STATE* state) {
  src_delete(state);
}

void drwav_free(drwav* wav) {
  drwav_uninit(wav);
}

result<audio_clip> drwav_read(drwav& wav) {
  audio_clip result;
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

result<audio_clip> drwav_load_memory(std::span<std::uint8_t> data) {
  drwav wav;
  if (!drwav_init_memory(&wav, data.data(), data.size(), /* allocation */ nullptr)) {
    return unexpected("Couldn't read in-memory wav");
  }
  auto wav_handle = make_raw(&wav, &drwav_free);
  return drwav_read(wav);
}

Mixer::audio_sample_t f2s16(float v) {
  return static_cast<Mixer::audio_sample_t>(
      std::floor(.5f + v * std::numeric_limits<Mixer::audio_sample_t>::max()));
}

}  // namespace

struct Mixer::impl_t {
  struct audio_resource {
    std::optional<audio_clip> clip;
  };
  audio_handle_t next_handle = 0;
  std::unordered_map<audio_handle_t, audio_resource> audio_resources;

  struct sound {
    std::span<const float> samples;
    raw_ptr<SRC_STATE> src_state;
    SRC_DATA src_data = {};
    float lvolume = 0.f;
    float rvolume = 0.f;
    std::size_t position = 0;
  };

  std::uint32_t sample_rate_hz = 0;
  std::atomic<float> master_volume{1.f};
  std::vector<sound> new_sounds;

  std::mutex sound_mutex;
  std::vector<sound> sounds;
  std::vector<float> resample_buffer;
  std::vector<float> mix_buffer;
};

Mixer::~Mixer() = default;

Mixer::Mixer(std::uint32_t sample_rate_hz) : impl_{std::make_unique<impl_t>()} {
  impl_->sample_rate_hz = sample_rate_hz;
}

void Mixer::set_master_volume(float volume) {
  impl_->master_volume = std::clamp(volume, 0.f, 1.f);
}

result<Mixer::audio_handle_t>
Mixer::load_wav_memory(std::span<std::uint8_t> data, std::optional<audio_handle_t> handle) {
  auto h = assign_handle(handle);
  if (!h) {
    return unexpected(h.error());
  }
  auto clip = drwav_load_memory(data);
  if (!clip) {
    return unexpected(clip.error());
  }
  impl_t::audio_resource r;
  r.clip = std::move(*clip);
  impl_->audio_resources.emplace(*h, std::move(r));
  return *h;
}

void Mixer::play(audio_handle_t handle, float volume, float pan, float pitch) {
  auto it = impl_->audio_resources.find(handle);
  if (it == impl_->audio_resources.end() || !it->second.clip) {
    return;
  }
  const auto& clip = *it->second.clip;

  volume = std::clamp(volume, 0.f, 1.f);
  pan = std::clamp(glm::pi<float>() / 4.f * (pan + 1.f), 0.f, glm::pi<float>() / 2.f);
  pitch = std::clamp(pitch, 1.f / 64, 64.f);

  int error = 0;
  impl_t::sound s;
  s.lvolume = std::clamp(volume * std::cos(pan), 0.f, 1.f);
  s.rvolume = std::clamp(volume * std::sin(pan), 0.f, 1.f);
  s.samples = clip.samples;
  s.src_data.src_ratio = static_cast<double>(impl_->sample_rate_hz) / clip.sample_rate_hz;
  s.src_data.src_ratio /= pitch;
  if (s.src_data.src_ratio == 1.f) {
    impl_->new_sounds.emplace_back(std::move(s));
  } else {
    s.src_state = make_raw(src_new(SRC_LINEAR, /* channels */ 1, &error), &src_free);
    if (s.src_state) {
      impl_->new_sounds.emplace_back(std::move(s));
    }
  }
}

void Mixer::commit() {
  std::unique_lock lock{impl_->sound_mutex};
  impl_->sounds.erase(
      std::remove_if(impl_->sounds.begin(), impl_->sounds.end(),
                     [](const impl_t::sound& s) { return s.position >= s.samples.size(); }),
      impl_->sounds.end());
  for (auto& s : impl_->new_sounds) {
    impl_->sounds.emplace_back(std::move(s));
  }
  impl_->new_sounds.clear();
}

void Mixer::audio_callback(std::uint8_t* out_buffer, std::size_t samples) {
  impl_->resample_buffer.resize(samples);
  impl_->mix_buffer.resize(2 * samples);
  std::fill(impl_->mix_buffer.begin(), impl_->mix_buffer.end(), 0.f);
  auto master_volume = impl_->master_volume.load();
  {
    std::unique_lock lock{impl_->sound_mutex};
    for (auto& s : impl_->sounds) {
      if (!s.src_state) {
        auto sample_count = std::min(samples, s.samples.size() - s.position);
        for (std::size_t i = 0; i < sample_count; ++i) {
          impl_->mix_buffer[2 * i] += s.lvolume * s.samples[s.position + i];
          impl_->mix_buffer[2 * i + 1] += s.rvolume * s.samples[s.position + i];
        }
        s.position += sample_count;
        continue;
      }

      s.src_data.data_in = s.samples.data() + s.position;
      s.src_data.input_frames = static_cast<long>(s.samples.size() - s.position);
      s.src_data.data_out = impl_->resample_buffer.data();
      s.src_data.output_frames = static_cast<long>(samples);
      s.src_data.end_of_input = 1;
      if (src_process(s.src_state.get(), &s.src_data)) {
        s.position = s.samples.size();
        continue;
      }

      auto sample_count = static_cast<std::size_t>(s.src_data.output_frames_gen);
      for (std::size_t i = 0; i < sample_count; ++i) {
        impl_->mix_buffer[2 * i] += s.lvolume * impl_->resample_buffer[i];
        impl_->mix_buffer[2 * i + 1] += s.rvolume * impl_->resample_buffer[i];
      }
      s.position += s.src_data.input_frames_used;
    }
  }
  for (auto& s : impl_->mix_buffer) {
    s *= master_volume;
  }
  for (std::size_t i = 0; i < samples; ++i) {
    auto v = std::max(std::abs(impl_->mix_buffer[2 * i]), std::abs(impl_->mix_buffer[2 * i + 1]));
    auto m = std::tanh(v) / v;
    auto l = f2s16(m * impl_->mix_buffer[2 * i]);
    auto r = f2s16(m * impl_->mix_buffer[2 * i + 1]);
    std::memcpy(out_buffer + 2 * i * sizeof(audio_sample_t), &l, sizeof(audio_sample_t));
    std::memcpy(out_buffer + (2 * i + 1) * sizeof(audio_sample_t), &r, sizeof(audio_sample_t));
  }
}

result<Mixer::audio_handle_t> Mixer::assign_handle(std::optional<audio_handle_t> requested) {
  auto handle = requested ? *requested : impl_->next_handle++;
  if (impl_->audio_resources.find(handle) != impl_->audio_resources.end()) {
    return unexpected("Audio handle already in use");
  }
  return handle;
}

}  // namespace ii