cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = ["//game/common:math"],
  implementation_deps = [
    "//game/logic/ecs",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic:__subpackages__"],
)