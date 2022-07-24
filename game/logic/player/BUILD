cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ship",
  ],
  implementation_deps = [
    ":ai_player",
    "//game/logic/ecs",
    "//game/logic/geometry/shapes",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "ai_player",
  hdrs = ["ai_player.h"],
  srcs = ["ai_player.cc"],
  deps = [
    "//game/logic/sim:sim_io",
    "//game/logic/ship",
  ],
  implementation_deps = [
    "//game/logic/ecs",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic:__subpackages__"],
)