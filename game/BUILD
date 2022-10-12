load("@rules_pkg//:pkg.bzl", "pkg_zip")
load("@bazel_skylib//rules:write_file.bzl", "write_file")

cc_library(
  name = "version",
  hdrs = ["version.h"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "flags",
  hdrs = ["flags.h"],
  deps = ["//game/common:types"],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "mode_flags",
  hdrs = ["mode_flags.h"],
  deps = [
    "//game:flags",
    "//game/common:types",
    "//game/logic/sim/io:conditions",
  ],
  visibility = ["//visibility:public"],
)

write_file(
  name = "steam_appid",
  out = "steam_appid.txt",
  content = ["2139740"],
)

cc_binary(
  name = "space",
  srcs = ["space.cc"],
  deps = [
    ":flags",
    ":mode_flags",
    "//game/core:game_options",
    "//game/core/layers:main_menu",
    "//game/core/sim:replay_viewer",
    "//game/core/sim:sim_layer",
    "//game/core/ui",
    "//game/io:sdl_io",
    "//game/io/file:std_filesystem",
    "//game/mixer",
    "//game/mixer:sound",
    "//game/render",
    "//game/system",
  ],
  data = select({
    "//:steam_build": [":steam_appid"],
    "//conditions:default": [],
  }),
  linkopts = select({
    "@bazel_tools//src/conditions:windows": [
      "-DEFAULTLIB:advapi32.lib",
      "-DEFAULTLIB:dxguid.lib",
      "-DEFAULTLIB:dinput8.lib",
      "-DEFAULTLIB:gdi32.lib",
      "-DEFAULTLIB:ole32.lib",
      "-DEFAULTLIB:oleaut32.lib",
      "-DEFAULTLIB:opengl32.lib",
      "-DEFAULTLIB:user32.lib",
      "-DEFAULTLIB:winmm.lib",
      "-DEFAULTLIB:xinput.lib",
    ],
    "//conditions:default": [],
  }),
  visibility = ["//visibility:public"],
)

pkg_zip(
  name = "game",
  srcs = [
    "//assets",
    "//game:space",
  ] + select({
    "@bazel_tools//src/conditions:windows": ["@sdl_windows//:sdl_windows_dll"],
    "//conditions:default": [],
  }) + select({
    "//:steam_build": ["@steamworks_sdk//:steamapi_bin"],
    "//conditions:default": [],
  }),
  package_dir = select({
    "//:steam_build": "",
    "//conditions:default": "space",
  }),
)

pkg_zip(
  name = "tools",
  srcs = [
    "//game/tools:ai_network_sim",
    "//game/tools:ai_replay_synth",
    "//game/tools:replay_network_sim",
    "//game/tools:replay",
  ],
  package_dir = "tools",
)
