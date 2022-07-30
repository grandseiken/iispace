cc_library(
  name = "io_input_adapter",
  hdrs = ["io_input_adapter.h"],
  srcs = ["io_input_adapter.cc"],
  deps = [
    "//game/logic/sim:sim_io",
    "@glm",
  ],
  implementation_deps = ["//game/io"],
)

cc_library(
  name = "ui_layer",
  hdrs = ["ui_layer.h"],
  srcs = ["ui_layer.cc"],
  deps = [
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
  name = "core",
  hdrs = [
    "modal.h",
    "z0_game.h",
  ],
  srcs = [
    "modal.cc",
    "z0_game.cc",
  ],
  deps = [
    ":io_input_adapter",
    "//game/data:replay",
    "//game/data:save",
    "//game/logic/sim",
  ],
  implementation_deps = [
    ":ui_layer",
    "//game/common:math",
    "//game/common:types",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer:sound",
    "//game/render:gl_renderer",
  ],
  visibility = ["//visibility:public"],
)
