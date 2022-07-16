cc_library(
  name = "logic",
  hdrs = [
    "stars.h",
    "boss/boss.h",
  ],
  srcs = [
    "boss/boss_internal.cc",
    "boss/boss_internal.h",
    "boss/chaser.cc",
    "boss/deathray.cc",
    "boss/ghost.cc",
    "boss/shield.cc",
    "boss/square.cc",
    "boss/super.cc",
    "boss/tractor.cc",
    "stars.cc",
  ],
  deps = [
    "@glm",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/logic/enemy",
    "//game/logic/player",
    "//game/mixer:sound",
  ],
  visibility = [":__subpackages__"],
)

cc_library(
  name = "overmind",
  hdrs = ["overmind.h"],
  srcs = ["overmind.cc"],
  implementation_deps = [
    ":logic",
    "//game/common:math",
    "//game/logic/enemy",
    "//game/logic/player",
  ],
  visibility = [":__subpackages__"],
)