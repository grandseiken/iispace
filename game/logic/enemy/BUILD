cc_library(
  name = "enemy",
  hdrs = ["enemy.h"],
  srcs = [
    "enemy_large.cc",
    "enemy_shots.cc",
    "enemy_small.cc",
    "enemy_snake.cc",
    "enemy_wall.cc",
  ],
  deps = ["//game/common:math"],
  implementation_deps = [
    "//game/logic/ecs",
    "//game/logic/ship",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic:__subpackages__"],
)