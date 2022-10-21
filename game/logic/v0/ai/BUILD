cc_library(
  name = "ai",
  hdrs = ["ai_player.h"],
  srcs = ["ai_player.cc"],
  deps = [
    "//game/logic/ecs",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = [
    "//game/logic/v0:components",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic/sim:__pkg__"],
)