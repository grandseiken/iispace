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
  deps = ["@glm"],
  defines = ["PLATFORM_64"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "ustring",
  hdrs = [
      "ustring.h",
      "ustring_convert.h",
  ],
  visibility = ["//visibility:public"],
)