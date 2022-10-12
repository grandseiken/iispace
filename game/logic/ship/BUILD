# TODO: sort components into legacy, non-legacy, shared, etc, and sort out
# uses by SimState.
cc_library(
  name = "components",
  hdrs = ["components.h"],
  srcs = ["components.cc"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/geometry",
    "//game/logic/sim/io:render",
    "//game/mixer:sound",
    "@glm",
    "@static_functional",
  ],
  implementation_deps = ["//game/logic/sim:sim_interface"],
  visibility = ["//game/logic:__subpackages__"],
)
