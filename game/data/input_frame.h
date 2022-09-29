#ifndef II_GAME_DATA_INPUT_FRAME_H
#define II_GAME_DATA_INPUT_FRAME_H
#include "game/data/proto/input_frame.pb.h"
#include "game/logic/sim/io/player.h"

namespace ii::data {

inline input_frame read_input_frame(const proto::InputFrame& proto) {
  input_frame frame;
  frame.velocity = {fixed::from_internal(proto.velocity_x()),
                    fixed::from_internal(proto.velocity_y())};
  vec2 target{fixed::from_internal(proto.target_x()), fixed::from_internal(proto.target_y())};
  if (proto.target_relative()) {
    frame.target_relative = target;
  } else {
    frame.target_absolute = target;
  }
  frame.keys = proto.keys();
  return frame;
}

inline proto::InputFrame write_input_frame(const input_frame& frame) {
  proto::InputFrame proto;
  proto.set_velocity_x(frame.velocity.x.to_internal());
  proto.set_velocity_y(frame.velocity.y.to_internal());
  if (frame.target_relative) {
    proto.set_target_relative(true);
    proto.set_target_x(frame.target_relative->x.to_internal());
    proto.set_target_y(frame.target_relative->y.to_internal());
  } else if (frame.target_absolute) {
    proto.set_target_x(frame.target_absolute->x.to_internal());
    proto.set_target_y(frame.target_absolute->y.to_internal());
  }
  proto.set_keys(frame.keys);
  return proto;
}

}  // namespace ii::data

#endif