cc_binary(
  name = "iispace",
  srcs = ["iispace.cc"],
  deps = [
    "//game/core",
    "//game/core:ui_layer",
    "//game/io:sdl_io",
    "//game/io/file:std_filesystem",
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
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "score",
  srcs = ["score.cc"],
  deps = [
    "//game/data:replay",
    "//game/io/file:std_filesystem",
    "//game/logic/sim",
  ],
  visibility = ["//visibility:public"],
)