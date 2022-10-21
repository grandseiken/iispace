cc_library(
  name = "components",
  hdrs = ["components.h"],
  srcs = ["components.cc"],
  deps = [
    "//game/common:math",
    "//game/common:types",
    "//game/logic/ecs",
    "//game/logic/sim:components",
  ],
  implementation_deps = ["//game/logic/sim:sim_interface"],
  visibility = [
    "//game/logic/legacy:__subpackages__",
    "//game/logic/sim:__pkg__",
  ],
)

cc_library(
  name = "setup",
  hdrs = ["setup.h"],
  srcs = ["setup.cc"],
  deps = ["//game/logic/sim:setup"],
  implementation_deps = [
    ":components",
    "//game/logic/legacy/overmind",
    "//game/logic/legacy/player",
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
    "//game/logic/sim:components",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:render",
    "//game/logic/v0:ship_template",
    "@static_functional",
  ],
  visibility = ["//game/logic/legacy:__subpackages__"],
)