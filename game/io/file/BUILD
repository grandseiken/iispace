cc_library(
  name = "filesystem",
  hdrs = ["filesystem.h"],
  deps = [
    "//game/common:types",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "std_filesystem",
  hdrs = ["std_filesystem.h"],
  srcs = ["std_filesystem.cc"],
  deps = [":filesystem"],
  visibility = ["//visibility:public"],
)