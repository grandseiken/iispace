cc_library(
  name = "input",
  hdrs = ["input.h"],
  deps = ["@glm"],
)

cc_library(
  name = "element",
  hdrs = ["element.h"],
  srcs = ["element.cc"],
  deps = [
    ":input",
    "//game/common:math",
    "@glm",
  ],
)

cc_library(
  name = "ui",
  hdrs = ["game_stack.h"],
  srcs = ["game_stack.cc"],
  deps = [
    ":input",
    ":element",
    "//game/data:config",
    "//game/data:replay",
    "//game/data:save",
    "//game/mixer:sound",
    "//game/render:gl_renderer"
  ],
  implementation_deps = [
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
  ],
  visibility = ["//visibility:public"],
)
