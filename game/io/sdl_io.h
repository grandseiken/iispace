#ifndef IISPACE_GAME_IO_SDL_IO_H
#define IISPACE_GAME_IO_SDL_IO_H
#include "game/common/result.h"
#include "game/io/io.h"
#include <glm/glm.hpp>
#include <memory>

namespace ii::io {

class SdlIoLayer : public IoLayer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<SdlIoLayer>>
  create(const char* title, char gl_major, char gl_minor);
  SdlIoLayer(access_tag);
  ~SdlIoLayer();

  glm::uvec2 dimensions() const override;
  void swap_buffers() override;
  void capture_mouse(bool capture) override;
  std::optional<event_type> poll() override;

  std::vector<controller::info> controller_info() const override;
  controller::frame controller_frame(std::size_t index) const override;
  keyboard::frame keyboard_frame() const override;
  mouse::frame mouse_frame() const override;
  void input_frame_clear() override;

  void set_audio_callback(const std::function<audio_callback>& callback) override;
  void close_audio_device() override;
  result<void> open_audio_device(std::optional<std::size_t> index) override;
  std::vector<std::string> audio_device_info() const override;

protected:
  void lock_audio_callback() override;
  void unlock_audio_callback() override;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::io

#endif