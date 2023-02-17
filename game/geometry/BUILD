cc_library(
  name = "geometry",
  hdrs = [
    "concepts.h",
    "expressions.h",
    "iteration.h",
    "node.h",
    "node_conditional.h",
    "node_for_each.h",
    "node_transform.h",
  ],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/geom2",
  ],
  visibility = ["//game:__subpackages__"],
)

cc_library(
  name = "resolve",
  hdrs = ["resolve.h"],
  deps = [
    ":geometry",
    "//game/geometry/shapes:headers",
  ],
  visibility = ["//game:__subpackages__"],
)