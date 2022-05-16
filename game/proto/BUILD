load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
  name = "iispace_proto",
  srcs = ["iispace.proto"],
)

cc_proto_library(
  name = "iispace_proto_cc",
  deps = [":iispace_proto"],
  visibility = ["//visibility:public"],
)