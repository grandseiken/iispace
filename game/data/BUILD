cc_library(
  name = "crypt",
  hdrs = ["crypt.h"],
  srcs = ["crypt.cc"],
  deps = [
    "//game/common:types",
    "@span_lite",
    "@zlib",
  ],
)

cc_library(
  name = "replay",
  hdrs = ["replay.h"],
  srcs = ["replay.cc"],
  deps = [
    ":crypt",
    "//game/common:math",
    "//game/common:types",
    "//game/data/proto:ii_proto_cc",
    "//game/logic/sim:sim_interface",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "save",
  hdrs = ["save.h"],
  srcs = ["save.cc"],
  deps = [
    ":crypt",
    "//game/common:types",
    "//game/data/proto:ii_proto_cc",
    "//game/logic/sim:sim_interface",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)