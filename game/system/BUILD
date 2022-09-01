cc_library(
  name = "system",
  srcs = ["system.cc"],
  deps = [":system_header"],
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
  name = "system_header",
  hdrs = ["system.h"],
  deps = ["//game/common:types"],
)

cc_library(
  name = "default",
  hdrs = ["default.h"],
)

cc_library(
  name = "steam",
  hdrs = ["steam.h"],
  srcs = ["steam.cc"],
  deps = [":system_header"],
  implementation_deps = ["@steamworks_sdk"],
  target_compatible_with = select({
    "@//:steam_build": [],
    "//conditions:default": ["@platforms//:incompatible"],
  }),
)