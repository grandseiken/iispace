# TODO: refactor this once IShip is no longer a thing.
# Probably split geometry.h out into a separate library with multiple headers.
cc_library(
  name = "ship",
  hdrs = [
    "components.h",
    "enums.h",
    "geometry.h",
    "shape.h",
    "ship_template.h",
    "ship.h",
  ],
  srcs = [
    "components.cc",
    "shape.cc",
    "ship.cc",
  ],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/sim:sim_interface",
    "@glm",
  ],
  visibility = ["//game/logic:__subpackages__"],
)
