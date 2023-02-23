cc_library(
  name = "components",
  hdrs = ["components.h"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/logic/ecs",
    "//game/logic/sim:components",
  ],
  visibility = [
    "//game/logic/sim:__pkg__",
    "//game/logic/v0:__subpackages__",
  ],
)

cc_library(
  name = "setup",
  hdrs = ["setup.h"],
  srcs = ["setup.cc"],
  deps = ["//game/logic/sim:setup"],
  implementation_deps = [
    ":components",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/overmind",
    "//game/logic/v0/player",
    "//game/logic/v0/player:powerup",
    "//game/logic/v0/player:shot",
  ],
  visibility = ["//game/logic/sim:__pkg__"],
)

cc_library(
  name = "ship_template",
  hdrs = [
    "particles.h",
    "ship_template.h",
  ],
  srcs = [
    "particles.cc",
    "ship_template.cc",
  ],
  deps = [
    ":components",
    "//game/common:math",
    "//game/geometry",
    "//game/logic/ecs",
    "//game/logic/sim:sim_interface",
    "@static_functional",
  ],
  visibility = ["//game/logic/v0:__subpackages__"],
)

cc_library(
  name = "waypoint",
  hdrs = ["waypoint.h"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/logic/sim:components",
    "//game/render/data",
  ],
  visibility = ["//game/logic/v0:__subpackages__"],
)