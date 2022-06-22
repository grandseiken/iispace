load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain", "proto_library")

proto_lang_toolchain(
  name = "cc_toolchain",
  command_line = "--cpp_out=lite:$(OUT)",
  runtime = "@com_github_protocolbuffers_protobuf//:protobuf_lite",
)

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