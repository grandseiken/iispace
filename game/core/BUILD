cc_library(
  name = "headers",
  hdrs = [
    "lib.h",
    "modal.h",
    "replay.h",
    "save.h",
    "z0_game.h",
  ],
  srcs = ["modal.cc"],
  deps = [
    "//game/common:types",
    "//game/common",
    "//game/mixer:sound",
    "//game/proto:iispace_proto_cc",
    "@span_lite",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "core",
  srcs = [
    "lib.cc",
    "replay.cc",
    "save.cc",
    "z0_game.cc",
  ],
  deps = [
    ":headers",
    "//game/io",
    "//game/io/file:filesystem",
    "//game/logic",
    "//game/mixer",
    "//game/proto:iispace_proto_cc",
    "//game/render:renderer",
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
  visibility = ["//visibility:public"],
)
