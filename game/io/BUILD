cc_library(
  name = "io",
  hdrs = [
    "input.h",
    "io.h",
  ],
  srcs = ["input.cc"],
  deps = [
    "//game/common:math",
    "//game/common:types",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sdl_io",
  hdrs = ["sdl_io.h"],
  srcs = [
    "sdl_convert.h",
    "sdl_io.cc",
  ],
  deps = [
    ":io",
    "//game/common:types",
  ],
  implementation_deps = [
    "//deps/sdl",
    "@gl3w",
    "@sdl_gamecontrollerdb",
    "@static_functional",
  ],
  visibility = ["//visibility:public"],
)