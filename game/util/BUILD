cc_library(
  name = "gamepad_header",
  hdrs = ["gamepad.h"],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "gamepad",
  srcs = ["gamepad.cc"],
  deps = [
    ":gamepad_header",
    "@ois",
  ],
  linkopts = select({
    "@bazel_tools//src/conditions:windows": [
      "-DEFAULTLIB:dxguid.lib",
      "-DEFAULTLIB:dinput8.lib",
      "-DEFAULTLIB:ole32.lib",
      "-DEFAULTLIB:oleaut32.lib",
      "-DEFAULTLIB:user32.lib",
    ],
  }),
)