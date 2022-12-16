cc_library(
  name = "enemy",
  hdrs = ["enemy.h"],
  srcs = [
    "biome0_large.cc",
    "biome0_small.cc",
    "biome0_wall.cc",
  ],
  deps = ["//game/common:math"],
  implementation_deps = [
    ":enemy_internal",
    "//game/geometry/shapes",
    "//game/logic/ecs",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0/overmind:__subpackages__"],
)

cc_library(
  name = "enemy_internal",
  hdrs = ["enemy_template.h"],
  deps = [
    "//game/logic/sim:sim_interface",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
    "@static_functional",
  ],
)