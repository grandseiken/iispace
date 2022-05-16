load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
  name = "iispace_proto",
  srcs = ["iispace.proto"],
)

cc_proto_library(
  name = "iispace_proto_cc",
  deps = [":iispace_proto"],
)

cc_library(
  name = "lib",
  hdrs = [
    "fix32.h",
    "lib.h",
    "modal.h",
    "replay.h",
    "save.h",
    "z.h",
    "z0_game.h",
  ],
  srcs = [
    "modal.cc",
    "z.cc",
  ],
  defines = ["PLATFORM_64"],
  deps = [
    ":iispace_proto_cc",
    "@zlib",
  ],
)

cc_library(
  name = "game",
  hdrs = [
    "boss.h",
    "boss_chaser.h",
    "overmind.h",
    "player.h",
    "shape.h",
    "ship.h",
    "stars.h",
  ],
  srcs = [
    "boss_chaser.cc",
    "boss_deathray.cc",
    "boss_deathray.h",
    "boss_ghost.cc",
    "boss_ghost.h",
    "boss_shield.cc",
    "boss_shield.h",
    "boss_square.cc",
    "boss_square.h",
    "boss_super.cc",
    "boss_super.h",
    "boss_tractor.cc",
    "boss_tractor.h",
    "boss.cc",
    "enemy.cc",
    "enemy.h",
    "overmind.cc",
    "player.cc",
    "shape.cc",
    "ship.cc",
    "stars.cc",
  ],
  deps = [":lib"],
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
    ":game",
    ":iispace_proto_cc",
    ":lib",
    "//src/util:gamepad_header",
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
    ":game",
    ":iispace_proto_cc",
    ":lib",
    "//src/util:gamepad_header",
  ],
)