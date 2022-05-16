#ifndef IISPACE_GAME_IO_SDL_IO_H
#define IISPACE_GAME_IO_SDL_IO_H
#include "game/common/result.h"
#include "game/io/io.h"
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

  void update() override;
  void swap_buffers() override;
  bool should_close() const override;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::io

#endif