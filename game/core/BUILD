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
    "//game/proto:iispace_proto_cc",
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
    "//game/logic",
    "//game/proto:iispace_proto_cc",
    "//game/util:gamepad_header",
    "@ois",
    "@sfml",
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
    "main.cc",
    "replay.cc",
    "save.cc",
    "z0_game.cc",
  ],
  defines = ["PLATFORM_SCORE"],
  deps = [
    ":lib",
    "//game/logic",
    "//game/proto:iispace_proto_cc",
    "//game/util:gamepad_header",
  ],
)