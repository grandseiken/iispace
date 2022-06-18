cc_library(
  # TODO
  name = "player",
  hdrs = ["player.h"],
  visibility = [":__subpackages__"],
)

cc_library(
  name = "logic",
  hdrs = [
    "overmind.h",
    "stars.h",
  ],
  srcs = [
    "boss/boss.h",
    "boss/boss_internal.cc",
    "boss/boss_internal.h",
    "boss/chaser.cc",
    "boss/deathray.cc",
    "boss/ghost.cc",
    "boss/shield.cc",
    "boss/square.cc",
    "boss/super.cc",
    "boss/tractor.cc",
    "overmind.cc",
    "player.cc",
    "stars.cc",
  ],
  deps = [
    ":player",
    "//game/common:math",
    "//game/logic/enemy",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
    "//game/mixer:sound",
    "@glm",
  ],
  visibility = [":__subpackages__"],
)