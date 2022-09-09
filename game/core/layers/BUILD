cc_library(
  name = "common",
  hdrs = ["common.h"],
  deps = ["@glm"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "main_menu",
  hdrs = ["main_menu.h"],
  srcs = ["main_menu.cc"],
  deps = [
    "//game/core:game_options",
    "//game/core/ui",
  ],
  implementation_deps = [
    ":common",
    "//game/core/sim:sim_layer",
    "//game/core/toolkit",
    "//game/io",
  ],
  visibility = ["//visibility:public"],
)