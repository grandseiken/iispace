cc_library(
  name = "sim_layer",
  hdrs = ["sim_layer.h"],
  srcs = ["sim_layer.cc"],
  deps = [
    "//game/core/ui",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = [
    ":hud_layer",
    ":input_adapter",
    ":pause_layer",
    ":render_state",
    "//game/core:game_options",
    "//game/core/layers:utility",
    "//game/data:packet",
    "//game/data:replay",
    "//game/data:save",
    "//game/logic/sim",
    "//game/logic/sim:networked_sim_state",
    "//game/mixer:sound",
    "//game/render",
    "//game/system:header",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "replay_viewer",
  hdrs = ["replay_viewer.h"],
  srcs = ["replay_viewer.cc"],
  deps = ["//game/core/ui"],
  implementation_deps = [
    ":hud_layer",
    ":pause_layer",
    ":render_state",
    "//game/core:game_options",
    "//game/data:replay",
    "//game/logic/sim",
    "//game/logic/sim:networked_sim_state",
    "//game/mixer:sound",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "hud_layer",
  hdrs = ["hud_layer.h"],
  srcs = ["hud_layer.cc"],
  deps = ["//game/core/ui"],
  implementation_deps = [
    "//game/core/layers:common",
    "//game/core/toolkit",
    "//game/logic/sim/io:output",
    "//game/render",
  ],
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
    "//game/io",
    "//game/system:header",
  ],
)

cc_library(
  name = "input_adapter",
  hdrs = ["input_adapter.h"],
  srcs = ["input_adapter.cc"],
  deps = [
    "//game/common:math",
    "//game/core/ui:input",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = ["//game/io"],
)

cc_library(
  name = "render_state",
  hdrs = ["render_state.h"],
  srcs = ["render_state.cc"],
  deps = [
    "//game/common:math",
    "//game/common:random",
    "//game/logic/sim/io:aggregate",
    "//game/render/data",
  ],
  implementation_deps = [
    ":input_adapter",
    "//game/logic/sim",
    "//game/logic/sim/io:output",
    "//game/mixer",
    "//game/render",
  ],
)