load("@rules_pkg//:pkg.bzl", "pkg_zip")

cc_library(
  name = "flags",
  hdrs = ["flags.h"],
  deps = ["//game/common:types"],
)

cc_library(
  name = "replay_tools",
  hdrs = ["replay_tools.h"],
  deps = [
    "//game/common:types",
    "//game/common:printer",
    "//game/data:replay",
    "//game/logic/sim",
    "//game/logic/sim/io:aggregate",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:results",
  ]
)

cc_library(
  name = "network_tools",
  hdrs = ["network_tools.h"],
  deps = [
    "//game:flags",
    "//game/common:types",
    "//game/data:packet",
    "//game/data:replay",
    "//game/logic/sim:networked_sim_state",
    "//game/logic/sim/io:aggregate",
    "@concurrent_queue",
  ],
)

cc_library(
  name = "mode_flags",
  hdrs = ["mode_flags.h"],
  deps = [
    "//game:flags",
    "//game/common:types",
    "//game/logic/sim/io:conditions",
  ],
)

cc_binary(
  name = "iispace",
  srcs = ["iispace.cc"],
  deps = [
    ":flags",
    ":mode_flags",
    "//game/core",
    "//game/core:ui_layer",
    "//game/io:sdl_io",
    "//game/io/file:std_filesystem",
    "//game/mixer",
    "//game/mixer:sound",
    "//game/render:gl_renderer",
  ],
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

cc_binary(
  name = "replay",
  srcs = ["replay.cc"],
  deps = [
    ":flags",
    ":replay_tools",
    "//game/data:replay",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "replay_network_sim",
  srcs = ["replay_network_sim.cc"],
  deps = [
    ":flags",
    ":network_tools",
    ":replay_tools",
    "//game/common:types",
    "//game/data:replay",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "ai_replay_synth",
  srcs = ["ai_replay_synth.cc"],
  deps = [
    ":flags",
    ":mode_flags",
    ":replay_tools",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "ai_network_sim",
  srcs = ["ai_network_sim.cc"],
  deps = [
    ":flags",
    ":mode_flags",
    ":network_tools",
    ":replay_tools",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

pkg_zip(
  name = "game",
  srcs = [
    "//assets",
    "//game:iispace",
  ] + select({
    "@bazel_tools//src/conditions:windows": ["@sdl_windows//:sdl_windows_dll"],
    "//conditions:default": [],
  }),
  package_dir = "iispace",
)

pkg_zip(
  name = "tools",
  srcs = [
    "//game:ai_network_sim",
    "//game:ai_replay_synth",
    "//game:replay_network_sim",
    "//game:replay",
  ],
  package_dir = "tools",
)
