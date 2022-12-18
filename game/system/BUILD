cc_library(
  name = "system",
  srcs = ["system.cc"],
  deps = [":header"],
  implementation_deps = [":default"] + select({
    "//:steam_build": [":steam"],
    "//conditions:default": [],
  }),
  copts = select({
    "//:steam_build": ["-DII_STEAM_SYSTEM"],
    "//conditions:default": [],
  }),
  visibility = ["//visibility:public"],
)

cc_library(
  name = "header",
  hdrs = ["system.h"],
  deps = [
    "//game/common:async",
    "//game/common:math",
    "//game/common:types",
    "//game/common:ustring",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "default",
  hdrs = ["default.h"],
  deps = [":header"],
)

cc_library(
  name = "steam",
  hdrs = ["steam.h"],
  srcs = ["steam.cc"],
  deps = [":header"],
  implementation_deps = [
    "@static_functional",
    "@steamworks_sdk",
  ],
  target_compatible_with = select({
    "@//:steam_build": [],
    "//conditions:default": ["@platforms//:incompatible"],
  }),
)