cc_library(
  name = "player",
  hdrs = ["player.h"],
  srcs = ["player.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    ":loadout",
    ":powerup",
    ":shot",
    "//game/geometry/shapes",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:player",
    "//game/logic/v0/lib:components",
    "//game/logic/v0/lib:ship_template",
  ],
  visibility = ["//game/logic/v0:__subpackages__"],
)

cc_library(
  name = "loadout",
  hdrs = [
    "loadout.h",
    "loadout_mods.h",
  ],
  srcs = [
    "loadout.cc",
    "loadout_mods.cc",
  ],
  deps = [
    "//game/common:ustring",
    "//game/logic/ecs",
    "//game/logic/sim/io:conditions",
    "//game/logic/v0/lib:components",
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/common:random",
  ],
  visibility = ["//game/logic/v0/overmind:__pkg__"],
)

cc_library(
  name = "powerup",
  hdrs = ["powerup.h"],
  srcs = ["powerup.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    ":loadout",
    "//game/geometry/shapes",
    "//game/logic/sim:sim_interface",
    "//game/logic/sim/io:conditions",
    "//game/logic/v0/lib:components",
    "//game/logic/v0/lib:ship_template",
  ],
  visibility = ["//game/logic/v0/lib:__pkg__"],
)

cc_library(
  name = "upgrade",
  hdrs = ["upgrade.h"],
  srcs = ["upgrade.cc"],
  implementation_deps = [
    ":loadout",
    "//game/core/icons",
    "//game/geometry",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0/lib:components",
    "//game/logic/v0/lib:ship_template",
  ],
  visibility = ["//game/logic/v0/overmind:__pkg__"],
)

cc_library(
  name = "shot",
  hdrs = ["shot.h"],
  srcs = ["shot.cc"],
  deps = [
    "//game/common:math",
    "//game/logic/ecs",
  ],
  implementation_deps = [
    ":loadout",
    "//game/geometry/shapes",
    "//game/logic/sim/io:conditions",
    "//game/logic/v0/lib:components",
    "//game/logic/v0/lib:ship_template",
  ],
)