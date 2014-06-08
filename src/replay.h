#ifndef IISPACE_SRC_REPLAY_H
#define IISPACE_SRC_REPLAY_H

#include "z.h"
#include <vector>

struct PlayerFrame {
  vec2 velocity;
  vec2 target;
  int32_t keys;
};

struct Replay {
  Replay(const std::string& path);
  Replay(int32_t players, Mode::mode mode, bool can_face_secret_boss);
  void record(const vec2& velocity, const vec2& target, int32_t keys);
  void end_recording(const std::string& name, int64_t score) const;

  bool recording;
  bool okay;
  std::string error;

  Mode::mode mode;
  int32_t players;
  int64_t seed;
  bool can_face_secret_boss;

  std::vector<PlayerFrame> player_frames;
};

const Replay& PlayRecording(const std::string& path);

#endif