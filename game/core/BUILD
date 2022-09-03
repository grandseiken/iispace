cc_library(
  name = "io_input_adapter",
  hdrs = ["io_input_adapter.h"],
  srcs = ["io_input_adapter.cc"],
  deps = [
    "//game/logic/sim/io:player",
    "@glm",
  ],
  implementation_deps = ["//game/io"],
)

cc_library(
  name = "game_stack",
  hdrs = ["game_stack.h"],
  srcs = ["game_stack.cc"],
  deps = [
    "//game/data:config",
    "//game/data:replay",
    "//game/data:save",
    "//game/mixer:sound",
  ],
  implementation_deps = [
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
  ],
  visibility = ["//visibility:public"],
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
    ":io_input_adapter",
    "//game/logic/sim",
    "//game/logic/sim/io:output",
    "//game/mixer",
    "//game/render:gl_renderer",
  ],
)

cc_library(
  name = "core",
  hdrs = ["game_layers.h"],
  srcs = ["game_layers.cc"],
  deps = [
    ":game_stack",
    ":io_input_adapter",
    ":render_state",
    "//game/data:replay",
    "//game/data:save",
    "//game/logic/sim",
    "//game/logic/sim:networked_sim_state",
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer:sound",
    "//game/render:gl_renderer",
  ],
  visibility = ["//visibility:public"],
)
