cc_library(
  name = "sim_interface",
  hdrs = ["sim_interface.h"],
  deps = [
    "//game/common:math",
    "//game/common:random",
    "//game/logic/ecs",
    "//game/logic/sim/io:aggregate",
    "//game/mixer:sound",
  ],
  visibility = ["//game/logic:__subpackages__"],
)

cc_library(
  name = "sim_internals",
  hdrs = [
    "collision.h",
    "sim_internals.h",
  ],
  srcs = ["collision.cc"],
  deps = [
    ":sim_interface",
    "//game/common:math",
    "//game/common:random",
    "//game/logic/ecs",
    "//game/logic/ship",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:output",
    "//game/logic/sim/io:player",
    "//game/logic/sim/io:render",
  ]
)

cc_library(
  name = "sim",
  hdrs = ["sim_state.h"],
  srcs = [
    "sim_interface.cc",
    "sim_state.cc",
  ],
  deps = ["//game/logic/sim/io:player"],
  implementation_deps = [
    ":sim_interface",
    ":sim_internals",
    "//game/common:math",
    "//game/data:replay",
    "//game/logic/legacy/overmind",
    "//game/logic/legacy/player",
    "//game/logic/ship",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
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
    "//game/logic/sim/io:output",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = ["//game/logic/sim/io:conditions"],
  visibility = ["//visibility:public"],
)