#ifndef IISPACE_SRC_REPLAY_H
#define IISPACE_SRC_REPLAY_H

#include "z.h"

struct PlayerFrame {
  vec2 velocity;
  vec2 target;
  int32_t keys;
};

struct Replay {
  Replay(const std::string& path);
  Replay(int32_t players, int32_t game_mode, bool can_face_secret_boss);
  void record(const vec2& velocity, const vec2& target, int32_t keys);
  void end_recording(const std::string& name, int64_t score) const;

  bool recording;
  bool okay;
  std::string error;

  int64_t seed;
  int32_t players;
  int32_t game_mode;
  bool can_face_secret_boss;

  std::vector<PlayerFrame> player_frames;
};

const Replay& PlayRecording(const std::string& path);

#endif