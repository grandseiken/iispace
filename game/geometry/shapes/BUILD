cc_library(
  name = "headers",
  hdrs = [
    "ball.h",
    "box.h",
    "data.h",
    "legacy.h",
    "line.h",
    "ngon.h",
  ],
  deps = [
    "//game/geometry",
    "//game/common:math",
    "//game/common:types",
    "//game/render/data",
  ],
  visibility = ["//game:__subpackages__"],
)

cc_library(
  name = "shapes",
  srcs = ["shapes.cc"],
  deps = [
    ":headers",
  ],
  visibility = ["//game:__subpackages__"],
)
