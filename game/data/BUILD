cc_library(
  name = "crypt",
  hdrs = ["crypt.h"],
  srcs = ["crypt.cc"],
  deps = ["//game/common:types"],
  implementation_deps = ["@zlib"],
)

cc_library(
  name = "packet",
  hdrs = ["packet.h"],
  deps = ["//game/logic/sim/io:player"],
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
    ":crypt",
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
    ":crypt",
    "//game/data/proto:ii_proto_cc",
    "//game/logic/sim/io:player",
  ],
  visibility = ["//visibility:public"],
)