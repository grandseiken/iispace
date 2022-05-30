load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
  name = "ii_proto",
  srcs = [
    "config.proto",
    "replay.proto",
    "savegame.proto",
  ],
)

cc_proto_library(
  name = "ii_proto_cc",
  deps = [":ii_proto"],
)

cc_library(
  name = "replay",
  hdrs = ["replay.h"],
  srcs = ["replay.cc"],
  deps = [
    ":ii_proto_cc",
    "//game/common:types",
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
    ":ii_proto_cc",
    "//game/common:types",
    "//game/common",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)