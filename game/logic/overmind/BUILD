cc_library(
  name = "overmind",
  hdrs = ["overmind.h"],
  srcs = ["overmind.cc"],
  deps = ["//game/common:types"],
  implementation_deps = [
    ":overmind_internal",
    "//game/common:math",
    "//game/logic/boss",
    "//game/logic/player",
    "//game/logic/sim:sim_interface",
  ],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "overmind_internal",
  hdrs = [
    "formations.h",
    "spawn_context.h",
    "stars.h",
  ],
  srcs = ["stars.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "//game/logic/enemy",
    "@glm",
  ],
)