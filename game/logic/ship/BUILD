# TODO: refactor this once IShip is no longer a thing.
# Probably split geometry.h out into a separate library with multiple headers.
cc_library(
  name = "ship",
  hdrs = [
    "components.h",
    "enums.h",
    "geometry.h",
    "ship_template.h",
  ],
  srcs = ["components.cc"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/sim:sim_interface",
    "@glm",
  ],
  visibility = ["//game/logic:__subpackages__"],
)
