cc_library(
  name = "overmind",
  hdrs = ["overmind.h"],
  srcs = ["overmind.cc"],
  implementation_deps = [
    ":biome",
    ":internal",
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:conditions",
    "//game/logic/v0:components",
    "//game/logic/v0/player",
  ],
  visibility = ["//game/logic/v0:__pkg__"],
)

cc_library(
  name = "biome",
  hdrs = ["biome.h"],
  srcs = ["biome.cc"],
  deps = [
    ":internal",
    "//game/logic/sim/io:conditions",
  ],
  implementation_deps = [
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/overmind/formations",
  ],
)

cc_library(
  name = "internal",
  hdrs = [
    "formation_set.h",
    "wave_data.h",
    "spawn_context.h",
  ],
  deps = [
    "//game/common:math",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:conditions",
    "@static_functional",
  ],
  visibility = ["//game/logic/v0/overmind:__subpackages__"]
)