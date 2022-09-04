cc_library(
  name = "common",
  hdrs = [
    "input.h",
    "rect.h",
  ],
  deps = ["@glm"],
)

cc_library(
  name = "element",
  hdrs = ["element.h"],
  srcs = ["element.cc"],
  deps = [
    ":common",
    "@glm",
  ],
)

cc_library(
  name = "ui",
  hdrs = ["game_stack.h"],
  srcs = ["game_stack.cc"],
  deps = [
    ":common",
    ":element",
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
