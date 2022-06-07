cc_library(
  name = "ecs",
  hdrs = ["ecs.h"],
  deps = ["//game/common:types"],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "ship",
  hdrs = [
    "gen_ship.h",
    "shape.h",
    "ship.h",
  ],
  srcs = [
    "gen_ship.cc",
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