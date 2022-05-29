cc_library(
  name = "types",
  hdrs = [
    "raw_ptr.h",
    "result.h",
  ],
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
  deps = [
    ":types",
    "@zlib",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)