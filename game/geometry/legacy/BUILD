cc_library(
  name = "legacy",
  hdrs = [
    "ball_collider.h",
    "box.h",
    "ngon.h",
    "polyarc.h",
  ],
  srcs = ["shapes.cc"],
  deps = [
    "//game/geometry",
    "//game/common:math",
    "//game/common:types",
    "//game/render/data",
  ],
  implementation_deps = ["//game/geometry/shapes"],
  visibility = ["//game/logic/legacy:__subpackages__"],
)
