cc_library(
  name = "replay",
  hdrs = ["replay.h"],
  srcs = ["replay.cc"],
  deps = [
    "//game/common:types",
    "//game/logic/sim:sim_interface",
    "//game/proto:iispace_proto_cc",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "core",
  hdrs = [
    "lib.h",
    "modal.h",
    "save.h",
    "z0_game.h",
  ],
  srcs = [
    "lib.cc",
    "modal.cc",
    "save.cc",
    "z0_game.cc",
  ],
  deps = [
    ":replay",
    "//game/common:types",
    "//game/common",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/logic/sim",
    "//game/mixer",
    "//game/mixer:sound",
    "//game/proto:iispace_proto_cc",
    "//game/render:gl_renderer",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)
