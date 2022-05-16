#ifndef IISPACE_GAME_IO_IO_H
#define IISPACE_GAME_IO_IO_H

namespace ii::io {

class IoLayer {
public:
  virtual ~IoLayer() = default;

  virtual void update() = 0;
  virtual void swap_buffers() = 0;
  virtual bool should_close() const = 0;
};

}  // namespace ii::io

#endif