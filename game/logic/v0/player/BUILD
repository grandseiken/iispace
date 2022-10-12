cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = [
    ":shot",
    "//game/logic/geometry/shapes",
    "//game/logic/ship:components",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0:__pkg__"],
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
    "//game/logic/ship:components",
    "//game/logic/sim/io:player",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0:__pkg__"],
)