cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    "//game/geometry/shapes",
    "//game/logic/legacy:components",
    "//game/logic/legacy:ship_template",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
  ],
  visibility = ["//game/logic/legacy:__pkg__"],
)

cc_library(
  name = "powerup",
  hdrs = ["powerup.h"],
  srcs = ["powerup.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/sim:components",
  ],
  implementation_deps = [
    "//game/geometry/shapes",
    "//game/logic/ecs",
    "//game/logic/legacy:components",
    "//game/logic/legacy:ship_template",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:player",
    "//game/logic/sim/io:conditions",
  ],
  visibility = [
    "//game/logic/legacy/boss:__pkg__",
    "//game/logic/legacy/overmind:__pkg__",
  ],
)