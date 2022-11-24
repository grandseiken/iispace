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
    ":components",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/geometry",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
    "@static_functional",
  ],
  visibility = ["//game/logic/v0:__subpackages__"],
)