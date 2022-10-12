cc_library(
  name = "setup",
  hdrs = ["setup.h"],
  srcs = ["setup.cc"],
  deps = ["//game/logic/sim:setup"],
  implementation_deps = [
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/overmind",
    "//game/logic/v0/player",
  ],
  visibility = ["//game/logic/sim:__pkg__"],
)

cc_library(
  name = "ship_template",
  hdrs = [
    "particles.h",
    "ship_template.h",
  ],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/geometry",
    "//game/logic/ship:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
    "@static_functional",
  ],
  visibility = [
    "//game/logic/legacy:__subpackages__",
    "//game/logic/v0:__subpackages__",
  ],
)