cc_library(
  name = "sim_io",
  hdrs = ["sim_io.h"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sim_interface",
  hdrs = ["sim_interface.h"],
  deps = [
    ":sim_io",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/mixer:sound",
  ],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "sim",
  hdrs = ["sim_state.h"],
  srcs = [
    "random_engine.cc",
    "random_engine.h",
    "sim_interface.cc",
    "sim_internals.h",
    "sim_state.cc",
  ],
  deps = [
    ":sim_io",
    "//game/mixer:sound",
  ],
  implementation_deps = [
    ":sim_interface",
    "//game/logic",
    "//game/logic:overmind",
    "//game/logic/player",
    "//game/logic/ship",
  ],
  visibility = ["//visibility:public"],
)