#ifndef II_GAME_IO_IO_H
#define II_GAME_IO_IO_H
#include "game/common/result.h"
#include "game/io/input.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ii::io {

enum class event_type {
  kClose,
  kControllerChange,
  kAudioDeviceChange,
};

static constexpr std::uint32_t kAudioSampleRate = 48000;
using audio_sample_t = std::int16_t;
using audio_callback = void(/* out buffer */ std::uint8_t*, /* samples */ std::size_t);

class IoLayer {
public:
  virtual ~IoLayer() = default;

  // Basic window functionality.
  virtual glm::uvec2 dimensions() const = 0;
  virtual void swap_buffers() = 0;
  virtual void capture_mouse(bool capture) = 0;
  virtual std::optional<event_type> poll() = 0;

  // Input.
  virtual std::size_t controllers() const = 0;
  virtual std::vector<controller::info> controller_info() const = 0;
  virtual controller::frame controller_frame(std::size_t index) const = 0;
  virtual keyboard::frame keyboard_frame() const = 0;
  virtual mouse::frame mouse_frame() const = 0;
  virtual void input_frame_clear() = 0;

  // Audio.
  virtual void set_audio_callback(const std::function<audio_callback>& callback) = 0;
  virtual void close_audio_device() = 0;
  virtual result<void> open_audio_device(std::optional<std::size_t> index) = 0;
  virtual std::vector<std::string> audio_device_info() const = 0;

  class audio_callback_lock_t {
  public:
    void lock() {
      layer_.lock_audio_callback();
    }

    void unlock() {
      layer_.unlock_audio_callback();
    }

  private:
    friend class IoLayer;

    audio_callback_lock_t(IoLayer& layer) : layer_{layer} {}
    IoLayer& layer_;
  };

  audio_callback_lock_t audio_callback_lock() {
    return {*this};
  }

protected:
  virtual void lock_audio_callback() = 0;
  virtual void unlock_audio_callback() = 0;
};

}  // namespace ii::io

#endif