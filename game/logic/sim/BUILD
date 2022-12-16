cc_library(
  name = "components",
  hdrs = ["components.h"],
  srcs = ["components.cc"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/geometry",
    "//game/logic/ecs",
    "//game/logic/sim/io:output",
    "//game/render/data",
    "//game/mixer:sound",
    "@static_functional",
  ],
  implementation_deps = [":sim_interface"],
  visibility = ["//game/logic:__subpackages__"],
)

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
    ":components",
    ":sim_interface",
    "//game/common:math",
    "//game/common:random",
    "//game/logic/ecs",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:output",
    "//game/logic/sim/io:player",
  ]
)

cc_library(
  name = "setup",
  hdrs = ["setup.h"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
  ],
  visibility = [
    "//game/logic/legacy:__pkg__",
    "//game/logic/v0:__pkg__",
  ],
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
    "//game/logic/legacy:components",
    "//game/logic/legacy:setup",
    "//game/logic/v0:components",
    "//game/logic/v0:setup",
    "//game/logic/v0/ai",
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