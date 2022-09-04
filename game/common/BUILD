cc_library(
  name = "math",
  hdrs = [
    "fix32.h",
    "math.h",
    "rect.h",
  ],
  deps = ["@glm"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "random",
  hdrs = ["random.h"],
  deps = [":math"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "types",
  hdrs = [
    "enum.h",
    "raw_ptr.h",
    "result.h",
    "struct_tuple.h",
  ],
  deps = ["@expected"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "printer",
  hdrs = ["printer.h"],
  deps = [
    ":math",
    ":types",
  ],
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
