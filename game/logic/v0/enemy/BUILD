cc_library(
  name = "enemy",
  hdrs = ["enemy.h"],
  srcs = [
    "biome0_large.cc",
    "biome0_small.cc",
    "biome0_wall.cc",
  ],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    ":enemy_internal",
    "//game/geom2",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/lib:ship_template",
  ],
  visibility = [
    "//game/logic/v0/boss:__subpackages__",
    "//game/logic/v0/overmind:__subpackages__",
  ],
)

cc_library(
  name = "enemy_internal",
  hdrs = ["enemy_template.h"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/lib:components",
    "//game/logic/v0/lib:ship_template",
    "@static_functional",
  ],
)