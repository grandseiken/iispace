cc_library(
  name = "core",
  hdrs = [
    "lib.h",
    "modal.h",
    "z0_game.h",
  ],
  srcs = [
    "lib.cc",
    "modal.cc",
    "z0_game.cc",
  ],
  deps = [
    "//game/common:types",
    "//game/common",
    "//game/data:replay",
    "//game/data:save",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/logic/sim",
    "//game/mixer",
    "//game/mixer:sound",
    "//game/render:gl_renderer",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)
