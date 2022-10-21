cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    ":bubble",
    ":shot",
    "//game/logic/geometry/shapes",
    "//game/logic/sim/io:player",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0:__subpackages__"],
)

cc_library(
  name = "bubble",
  hdrs = ["bubble.h"],
  srcs = ["bubble.cc"],
  deps = ["//game/logic/ecs"],
  implementation_deps = [
    "//game/logic/geometry/shapes",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:player",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
)

cc_library(
  name = "powerup",
  hdrs = ["powerup.h"],
  srcs = ["powerup.cc"],
  deps = ["//game/common:math"],
  implementation_deps = [
    "//game/logic/sim/io:player",
    "//game/logic/ecs",
    "//game/logic/geometry/shapes",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic/v0/enemy:__subpackages__"],
)

cc_library(
  name = "shot",
  hdrs = ["shot.h"],
  srcs = ["shot.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    "//game/logic/geometry/shapes",
    "//game/logic/sim/io:player",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
)