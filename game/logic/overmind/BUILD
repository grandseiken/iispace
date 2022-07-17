cc_library(
  name = "overmind",
  hdrs = ["overmind.h"],
  srcs = [
    "overmind.cc",
    "stars.cc",
    "stars.h",
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/logic/boss",
    "//game/logic/enemy",
    "//game/logic/player",
  ],
  visibility = ["//game/logic:__subpackages__"],
)