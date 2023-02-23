cc_library(
  name = "geometry",
  hdrs = [
    "resolve.h",
    "shape_bank.h",
    "shape_data.h",
    "types.h",
    "value_parameters.h",
  ],
  srcs = ["shape_bank.cc"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/render/data",
    "@static_functional",
  ],
  visibility = ["//game:__subpackages__"],
)