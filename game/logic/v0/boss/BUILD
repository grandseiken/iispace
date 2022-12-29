cc_library(
  name = "boss",
  hdrs = ["boss.h"],
  srcs = ["biome0_square.cc"],
  implementation_deps = [
    ":boss_internal",
    "//game/geometry/shapes",
    "//game/logic/ecs",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/enemy",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0/overmind:__subpackages__"],
)

cc_library(
  name = "boss_internal",
  hdrs = ["boss_template.h"],
  deps = [
    "//game/geometry",
    "//game/logic/ecs",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
)