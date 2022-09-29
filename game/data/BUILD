cc_library(
  name = "internal",
  hdrs = [
    "conditions.h",
    "crypt.h",
    "input_frame.h",
    "proto_tools.h",
  ],
  srcs = ["crypt.cc"],
  deps = [
    "//game/common:types",
    "//game/data/proto:ii_proto_cc",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = ["@zlib"],
)

cc_library(
  name = "packet",
  hdrs = ["packet.h"],
  srcs = ["packet.cc"],
  deps = [
    ":internal",
    "//game/common:types",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = ["//game/data/proto:ii_proto_cc"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "replay",
  hdrs = ["replay.h"],
  srcs = ["replay.cc"],
  deps = [
    "//game/common:types",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
  ],
  implementation_deps = [
    ":internal",
    "//game/common:math",
    "//game/data/proto:ii_proto_cc",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "save",
  hdrs = ["save.h"],
  srcs = ["save.cc"],
  deps = [
    "//game/common:types",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:output",
  ],
  implementation_deps = [
    ":internal",
    "//game/data/proto:ii_proto_cc",
    "//game/logic/sim/io:player",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "config",
  hdrs = ["config.h"],
  srcs = ["config.cc"],
  deps = ["//game/common:types"],
  implementation_deps = [
    ":internal",
    "//game/data/proto:ii_proto_cc",
  ],
  visibility = ["//visibility:public"],
)