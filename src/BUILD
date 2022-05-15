load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
  name = "z",
  includes = [
    "fix32.h",
    "z.h",
  ],
  defines = ["PLATFORM_64"],
)

proto_library(
  name = "iispace_proto",
  srcs = ["iispace.proto"],
)

cc_proto_library(
  name = "iispace_proto_cc",
  deps = [":iispace_proto"],
)

cc_library(
  name = "lib",
  includes = [
    "lib.h",
    "save.h",
  ],
  srcs = [
    "lib.cpp",
    "save.cpp",
  ],
  deps = [
    ":iispace_proto_cc",
    ":z",
    "//src/util:gamepad_header",
    "@ois",
    "@sfml",
  ],
)
