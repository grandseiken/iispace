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
  name = "background",
  hdrs = ["background.h"],
  srcs = ["background.cc"],
  deps = [
    "//game/common:math",
    "//game/common:random",
    "//game/render/data",
  ],
)

cc_library(
  name = "ui",
  hdrs = ["game_stack.h"],
  srcs = ["game_stack.cc"],
  deps = [
    ":input",
    ":input_adapter",
    ":element",
    "//game/common:random",
    "//game/core:game_options",
    "//game/data:config",
    "//game/data:replay",
    "//game/data:save",
    "//game/mixer:sound",
    "//game/render/data",
  ],
  implementation_deps = [
    ":background",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
)
