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
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
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
  ],
  implementation_deps = [
    "//game/common:math",
    "//game/common:random",
    "//game/logic/sim/io:conditions",
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
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
  visibility = ["//game/logic/v0:__pkg__"],
)

cc_library(
  name = "upgrade",
  hdrs = ["upgrade.h"],
  srcs = ["upgrade.cc"],
  implementation_deps = [
    ":loadout",
    "//game/core/icons",
    "//game/logic/sim:sim_interface",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
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
    "//game/geometry/shapes",
    "//game/logic/sim/io:conditions",
    "//game/logic/v0:components",
    "//game/logic/v0:ship_template",
  ],
)