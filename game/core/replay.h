#ifndef IISPACE_GAME_CORE_REPLAY_H
#define IISPACE_GAME_CORE_REPLAY_H
#include "game/common/z.h"
#include "game/proto/iispace.pb.h"

struct Replay {
  Replay(const std::string& path);
  Replay(game_mode mode, std::int32_t players, bool can_face_secret_boss);
  void record(const vec2& velocity, const vec2& target, std::int32_t keys);
  void write(const std::string& name, std::int64_t score) const;

  proto::Replay replay;
};

#endif
