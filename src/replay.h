#ifndef IISPACE_SRC_REPLAY_H
#define IISPACE_SRC_REPLAY_H

#include "z.h"
#include "../gen/iispace.pb.h"

struct Replay {
  Replay(const std::string& path);
  Replay(int32_t players, Mode::mode mode, bool can_face_secret_boss);
  void record(const vec2& velocity, const vec2& target, int32_t keys);
  void end_recording(const std::string& name, int64_t score) const;

  bool recording;
  bool okay;
  std::string error;
  proto::Replay replay;
};

#endif