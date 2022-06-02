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
  visibility = ["//game/data:__pkg__"],
)