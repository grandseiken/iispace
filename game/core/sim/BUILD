cc_library(
  name = "sim_layer",
  hdrs = ["sim_layer.h"],
  srcs = ["sim_layer.cc"],
  deps = [
    "//game/core/ui",
  ],
  implementation_deps = [
    ":input_adapter",
    ":pause_layer",
    ":render_state",
    "//game/common:math",
    "//game/common:types",
    "//game/core:game_options",
    "//game/core/toolkit",
    "//game/data:replay",
    "//game/data:save",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/logic/sim",
    "//game/logic/sim:networked_sim_state",
    "//game/mixer:sound",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "pause_layer",
  hdrs = ["pause_layer.h"],
  srcs = ["pause_layer.cc"],
  deps = ["//game/core/ui"],
  implementation_deps = [
    "//game/core/layers:common",
    "//game/core/toolkit",
    "//game/render",
  ],
)

cc_library(
  name = "input_adapter",
  hdrs = ["input_adapter.h"],
  srcs = ["input_adapter.cc"],
  deps = [
    "//game/logic/sim/io:player",
    "@glm",
  ],
  implementation_deps = ["//game/io"],
)

cc_library(
  name = "render_state",
  hdrs = ["render_state.h"],
  srcs = ["render_state.cc"],
  deps = [
    "//game/common:random",
    "//game/logic/sim/io:aggregate",
    "@glm",
  ],
  implementation_deps = [
    ":input_adapter",
    "//game/logic/sim",
    "//game/logic/sim/io:output",
    "//game/mixer",
    "//game/render",
  ],
)