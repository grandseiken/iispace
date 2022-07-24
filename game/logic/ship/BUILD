cc_library(
  name = "ship",
  hdrs = [
    "components.h",
    "ship_template.h",
  ],
  srcs = ["components.cc"],
  deps = [
    "//game/common:types",
    "//game/common:math",
    "//game/logic/ecs",
    "//game/logic/geometry",
    "//game/logic/sim:sim_interface",
    "@glm",
  ],
  visibility = ["//game/logic:__subpackages__"],
)
