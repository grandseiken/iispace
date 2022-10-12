cc_library(
  name = "setup",
  hdrs = ["setup.h"],
  srcs = ["setup.cc"],
  deps = ["//game/logic/sim:setup"],
  implementation_deps = [
    "//game/logic/legacy/overmind",
    "//game/logic/legacy/player",
    "//game/logic/ship:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:events",
  ],
  visibility = ["//game/logic/sim:__pkg__"],
)

cc_library(
  name = "ship_template",
  hdrs = ["ship_template.h"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/geometry",
    "//game/logic/ship:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
    "//game/logic/v0:ship_template",
    "@static_functional",
  ],
  visibility = ["//game/logic/legacy:__subpackages__"],
)