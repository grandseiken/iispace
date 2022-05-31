cc_library(
  name = "io_input_adapter",
  hdrs = ["io_input_adapter.h"],
  srcs = ["io_input_adapter.cc"],
  deps = [
    "//game/data:replay",
    "//game/logic/sim:sim_interface",
    "//game/io",
    "@glm",
  ],
)

cc_library(
  name = "ui_layer",
  hdrs = ["ui_layer.h"],
  srcs = ["ui_layer.cc"],
  deps = [
    "//game/data:replay",
    "//game/data:save",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
    "//game/mixer:sound",
    "@span_lite",
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
    ":ui_layer",
    "//game/common:types",
    "//game/common",
    "//game/data:replay",
    "//game/data:save",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/logic/sim",
    "//game/mixer:sound",
    "//game/render:gl_renderer",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)
