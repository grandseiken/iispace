cc_library(
  name = "conditions",
  hdrs = ["conditions.h"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/common:ustring",
    "@glm",
  ],
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
  name = "aggregate",
  hdrs = ["aggregate.h"],
  deps = [
    "//game/mixer:sound",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "events",
  hdrs = ["events.h"],
  deps = ["//game/common:types"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "output",
  hdrs = ["output.h"],
  deps = [
    ":aggregate",
    ":events",
    "//game/common:types",
    "//game/render/data",
  ],
  visibility = ["//visibility:public"],
)