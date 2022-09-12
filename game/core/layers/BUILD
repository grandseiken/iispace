cc_library(
  name = "common",
  hdrs = ["common.h"],
  srcs = ["common.cc"],
  deps = ["@glm"],
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
    "//game/core:game_options",
    "//game/core/sim:sim_layer",
    "//game/core/toolkit",
    "//game/io",
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
    "//game/core/toolkit",
  ],
  visibility = ["//visibility:public"],
)