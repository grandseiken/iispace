cc_library(
  name = "input",
  hdrs = ["input.h"],
  deps = [
    "//game/common:math",
    "//game/mixer:sound",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "element",
  hdrs = ["element.h"],
  srcs = ["element.cc"],
  deps = [
    "//game/common:math",
    "//game/common:types",
  ],
  implementation_deps = [
    ":input",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "input_adapter",
  hdrs = ["input_adapter.h"],
  srcs = ["input_adapter.cc"],
  deps = [
    ":input",
    "//game/common:math",
  ],
  implementation_deps = ["//game/io"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "ui",
  hdrs = ["game_stack.h"],
  srcs = ["game_stack.cc"],
  deps = [
    ":input",
    ":input_adapter",
    ":element",
    "//game/core:game_options",
    "//game/data:config",
    "//game/data:replay",
    "//game/data:save",
    "//game/mixer:sound",
    "//game/render"
  ],
  implementation_deps = [
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
  ],
  visibility = ["//visibility:public"],
)
