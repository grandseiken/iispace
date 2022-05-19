cc_library(
  name = "io",
  hdrs = [
    "input.h",
    "io.h",
  ],
  srcs = ["input.cc"],
  deps = ["@glm"],
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
    "//deps/sdl",
    "//game/common:types",
    "@gl3w",
    "@sdl_gamecontrollerdb",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "sdl_example",
  srcs = ["sdl_example.cc"],
  deps = [":sdl_io"],
)