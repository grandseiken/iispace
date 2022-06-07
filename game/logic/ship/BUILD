cc_library(
  name = "ecs",
  hdrs = [
    "ecs_id.h",
    "ecs_index.h",
    "ecs_index_impl.h",
  ],
  deps = ["//game/common:types"],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "ship",
  hdrs = [
    "shape.h",
    "ship.h",
  ],
  srcs = [
    "shape.cc",
    "ship.cc",
  ],
  deps = [
    ":ecs",
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "@glm",
  ],
  visibility = ["//game/logic:__subpackages__"],
)