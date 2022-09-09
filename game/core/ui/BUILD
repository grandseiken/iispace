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
    "//game/common:math",
    "//game/common:types",
    "@glm",
  ],
  implementation_deps = [
    ":input",
    "//game/render",
  ],
)

cc_library(
  name = "toolkit",
  hdrs = ["toolkit.h"],
  srcs = ["toolkit.cc"],
  deps = [
    ":element",
    "//game/common:ustring",
    "//game/render:render_common",
  ],
  implementation_deps = [
    ":input",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
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
    "//game/render"
  ],
  implementation_deps = [
    "//game/io",
    "//game/io/file:filesystem",
    "//game/mixer",
  ],
  visibility = ["//visibility:public"],
)
