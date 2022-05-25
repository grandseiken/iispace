cc_library(
  name = "lib",
  hdrs = [
    "lib.h",
    "modal.h",
    "replay.h",
    "save.h",
    "z0_game.h",
  ],
  srcs = ["modal.cc"],
  deps = [
    "//game/common",
    "//game/io",
    "//game/mixer",
    "//game/proto:iispace_proto_cc",
    "//game/render:renderer",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "iispace",
  srcs = [
    "lib.cc",
    "main.cc",
    "replay.cc",
    "save.cc",
    "z0_game.cc",
  ],
  deps = [
    ":lib",
    "//game/io:sdl_io",
    "//game/logic",
    "//game/proto:iispace_proto_cc",
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
  }),
)

cc_binary(
  name = "score",
  srcs = [
    "lib.cc",
    "main_score.cc",
    "replay.cc",
    "save.cc",
    "z0_game.cc",
  ],
  defines = ["PLATFORM_SCORE"],
  deps = [
    ":lib",
    "//game/io:null_io",
    "//game/logic",
    "//game/proto:iispace_proto_cc",
    "//game/render:null_renderer",
  ],
)