cc_library(
  name = "conditions",
  hdrs = ["conditions.h"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/common:ustring",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "player",
  hdrs = ["player.h"],
  deps = ["//game/common:math"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "aggregate",
  hdrs = ["aggregate.h"],
  deps = [
    "//game/common:math",
    "//game/mixer:sound",
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
    "//game/common:ustring",
    "//game/render/data",
  ],
  visibility = ["//visibility:public"],
)