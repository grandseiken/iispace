cc_library(
  name = "legacy",
  hdrs = [
    "attachment_point.h",
    "ball_collider.h",
    "box.h",
    "line.h",
    "ngon.h",
    "polyarc.h",
  ],
  deps = [
    "//game/logic/geometry",
    "//game/logic/sim/io:render",
    "//game/common:types",
    "//game/common:math",
    "@glm",
  ],
  # TODO: restrict to //game/logic/legacy.
  visibility = ["//game/logic:__subpackages__"],
)
