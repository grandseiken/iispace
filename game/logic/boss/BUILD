cc_library(
  name = "boss",
  hdrs = ["boss.h"],
  srcs = [
    "boss_internal.cc",
    "boss_internal.h",
    "chaser.cc",
    "deathray.cc",
    "ghost.cc",
    "shield.cc",
    "square.cc",
    "super.cc",
    "tractor.cc",
  ],
  deps = [
    "@glm",
    "//game/logic/geometry/shapes",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/logic/enemy",
    "//game/logic/player",
    "//game/logic/sim/io:result_events",
    "//game/mixer:sound",
  ],
  visibility = ["//game/logic:__subpackages__"],
)