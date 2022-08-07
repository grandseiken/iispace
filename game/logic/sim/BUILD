cc_library(
  name = "sim_io",
  hdrs = ["sim_io.h"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/mixer:sound",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sim_interface",
  hdrs = [
    "random_engine.h",
    "sim_interface.h",
  ],
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
    "sim_interface.cc",
    "sim_internals.h",
    "sim_state.cc",
  ],
  deps = [
    ":sim_io",
  ],
  implementation_deps = [
    ":sim_interface",
    "//game/data:replay",
    "//game/logic/overmind",
    "//game/logic/player",
    "//game/logic/ship",
    "//game/logic/sim/fx",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "networked_sim_state",
  hdrs = ["networked_sim_state.h"],
  srcs = ["networked_sim_state.cc"],
  deps = [
    ":sim",
    "//game/data:packet",
  ],
  visibility = ["//visibility:public"],
)