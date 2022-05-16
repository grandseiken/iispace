cc_library(
  name = "result",
  hdrs = ["result.h"],
  deps = ["@expected"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "common",
  hdrs = [
    "fix32.h",
    "z.h",
  ],
  srcs = ["z.cc"],
  defines = ["PLATFORM_64"],
  deps = ["@zlib"],
  visibility = ["//visibility:public"],
)