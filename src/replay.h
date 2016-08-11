#ifndef IISPACE_SRC_REPLAY_H
#define IISPACE_SRC_REPLAY_H

#include <iispace.pb.h>
#include "z.h"

struct Replay {
  Replay(const std::string& path);
  Replay(Mode::mode mode, int32_t players, bool can_face_secret_boss);
  void record(const vec2& velocity, const vec2& target, int32_t keys);
  void write(const std::string& name, int64_t score) const;

  proto::Replay replay;
};

#endif
