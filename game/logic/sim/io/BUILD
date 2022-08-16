cc_library(
  name = "aggregate",
  hdrs = ["aggregate.h"],
  deps = [
    "//game/mixer:sound",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "conditions",
  hdrs = ["conditions.h"],
  deps = ["//game/common:types"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "player",
  hdrs = ["player.h"],
  deps = [
    "//game/common:math",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "render",
  hdrs = ["render.h"],
  deps = ["@glm"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "result_events",
  hdrs = ["result_events.h"],
  deps = ["//game/common:types"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "results",
  hdrs = ["results.h"],
  deps = [
    ":result_events",
    "//game/common:types",
  ],
  visibility = ["//visibility:public"],
)