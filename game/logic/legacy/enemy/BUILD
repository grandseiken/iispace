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
    "//game/logic/geometry/legacy",
    "//game/logic/legacy:components",
    "//game/logic/legacy:ship_template",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
  ],
  visibility = [
    "//game/logic/legacy/boss:__pkg__",
    "//game/logic/legacy/overmind:__pkg__",
  ],
)