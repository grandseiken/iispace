cc_library(
  name = "enemy",
  hdrs = ["enemy.h"],
  srcs = ["enemy_small.cc"],
  deps = ["//game/common:math"],
  implementation_deps = [
    ":enemy_internal",
    "//game/logic/ecs",
    "//game/logic/geometry/shapes",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0/overmind:__pkg__"],
)

cc_library(
  name = "enemy_internal",
  hdrs = ["enemy_template.h"],
  deps = [
    "//game/logic/ship:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0:ship_template",
    "@static_functional",
  ],
)