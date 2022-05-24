#ifndef IISPACE_GAME_IO_NULL_IO_H
#define IISPACE_GAME_IO_NULL_IO_H
#include "game/common/result.h"
#include "game/io/io.h"

namespace ii::io {

class NullIoLayer : public IoLayer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<NullIoLayer>> create() {
    return std::make_unique<NullIoLayer>(access_tag{});
  }
  NullIoLayer(access_tag) {}
  ~NullIoLayer() override {}

  glm::uvec2 dimensions() const override {
    return {0, 0};
  }
  void swap_buffers() override {}
  void capture_mouse(bool) override {}
  std::optional<event_type> poll() override {
    return std::nullopt;
  }

  std::vector<controller::info> controller_info() const override {
    return {};
  }
  controller::frame controller_frame(std::size_t) const override {
    return {};
  }
  keyboard::frame keyboard_frame() const override {
    return {};
  }
  mouse::frame mouse_frame() const override {
    return {};
  }
  void input_frame_clear() override {}

  void set_audio_callback(const std::function<audio_callback>&) override {}
  void close_audio_device() override {}
  result<void> open_audio_device(std::optional<std::size_t> index) override {
    return unexpected("No audio device");
  }
  std::vector<std::string> audio_device_info() const {
    return {};
  }

protected:
  void lock_audio_callback() override {}
  void unlock_audio_callback() override {}
};

}  // namespace ii::io

#endif