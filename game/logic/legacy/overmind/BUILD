cc_library(
  name = "overmind",
  hdrs = ["overmind.h"],
  srcs = ["overmind.cc"],
  implementation_deps = [
    ":overmind_internal",
    "//game/common:math",
    "//game/common:types",
    "//game/logic/legacy/boss",
    "//game/logic/ecs",
    "//game/logic/legacy/player:powerup",
    "//game/logic/ship:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:conditions",
  ],
  visibility = ["//game/logic/legacy:__pkg__"],
)

cc_library(
  name = "overmind_internal",
  hdrs = [
    "formations.h",
    "spawn_context.h",
  ],
  deps = [
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "//game/logic/legacy/enemy",
    "@glm",
  ],
)