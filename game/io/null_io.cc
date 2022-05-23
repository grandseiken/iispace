#include "game/io/null_io.h"

namespace ii::io {

result<std::unique_ptr<NullIoLayer>> NullIoLayer::create() {
  return std::make_unique<NullIoLayer>(access_tag{});
}

NullIoLayer::NullIoLayer(access_tag) {}

NullIoLayer::~NullIoLayer() {}

glm::uvec2 NullIoLayer::dimensions() const {
  return {0, 0};
}

void NullIoLayer::swap_buffers() {}

void NullIoLayer::capture_mouse(bool) {}

std::optional<event_type> NullIoLayer::poll() {
  return std::nullopt;
}

std::vector<controller::info> NullIoLayer::controller_info() const {
  return {};
}

controller::frame NullIoLayer::controller_frame(std::size_t) const {
  return {};
}

keyboard::frame NullIoLayer::keyboard_frame() const {
  return {};
}

mouse::frame NullIoLayer::mouse_frame() const {
  return {};
}

void NullIoLayer::input_frame_clear() {}

void NullIoLayer::set_audio_callback(const std::function<audio_callback>&) {}

void NullIoLayer::close_audio_device() {}

result<void> NullIoLayer::open_audio_device(std::optional<std::size_t>) {
  return unexpected("No audio device");
}

std::vector<std::string> NullIoLayer::audio_device_info() const {
  return {};
}

void NullIoLayer::lock_audio_callback() {}

void NullIoLayer::unlock_audio_callback() {}

}  // namespace ii::io