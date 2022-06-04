cc_library(
  name = "ship",
  hdrs = [
    "gen_ship.h",
    "shape.h",
    "ship.h",
    "ship_data.h",
  ],
  srcs = [
    "gen_ship.cc",
    "shape.cc",
    "ship.cc",
    "ship_data.cc",
  ],
  deps = [
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "@glm",
  ],
  visibility = ["//game/logic:__subpackages__"],
)