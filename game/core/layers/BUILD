cc_library(
  name = "common",
  hdrs = ["common.h"],
  srcs = ["common.cc"],
  deps = ["@glm"],
  implementation_deps = ["//game/core/toolkit"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "utility",
  hdrs = ["utility.h"],
  srcs = ["utility.cc"],
  deps = [
    ":common",
    "//game/common:async",
    "//game/common:ustring",
    "//game/core/toolkit",
    "//game/core/ui",
  ],
  implementation_deps = ["//game/core/toolkit"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "main_menu",
  hdrs = ["main_menu.h"],
  srcs = ["main_menu.cc"],
  deps = ["//game/core/ui"],
  implementation_deps = [
    ":common",
    ":run_lobby",
    ":utility",
    "//game:version",
    "//game/core:game_options",
    "//game/core/sim:sim_layer",
    "//game/core/toolkit",
    "//game/io",
    "//game/system:header",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "run_lobby",
  hdrs = ["run_lobby.h"],
  srcs = ["run_lobby.cc"],
  deps = [
    "//game/core/ui",
    "//game/logic/sim/io:conditions",
  ],
  implementation_deps = [
    ":common",
    ":slot_coordinator",
    ":utility",
    "//game/core/sim:sim_layer",
    "//game/core/toolkit",
    "//game/data:packet",
    "//game/logic/sim/io:player",
    "//game/io",
    "//game/system:header",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "slot_coordinator",
  hdrs = ["slot_coordinator.h"],
  srcs = ["slot_coordinator.cc"],
  deps = [
    "//game/core/ui",
    "//game/data:packet",
  ],
  implementation_deps = [
    ":common",
    "//game/core/toolkit",
  ],
)