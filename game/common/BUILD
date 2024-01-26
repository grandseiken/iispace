cc_library(
  name = "async",
  hdrs = ["async.h"],
  deps = [":types"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "math",
  hdrs = [
    "colour.h",
    "collision.h",
    "easing.h",
    "fix32.h",
    "math.h",
    "rect.h",
  ],
  srcs = ["fix32.cc"],
  deps = [
    "@gcem",
    "@glm",
  ],
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
    "variant_switch.h",
  ],
  deps = ["@static_functional"],
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
  srcs = ["ustring.cc"],
  visibility = ["//visibility:public"],
)
