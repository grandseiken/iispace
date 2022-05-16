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
    "modal.cpp",
    "z.cpp",
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
    "boss_chaser.cpp",
    "boss_deathray.cpp",
    "boss_deathray.h",
    "boss_ghost.cpp",
    "boss_ghost.h",
    "boss_shield.cpp",
    "boss_shield.h",
    "boss_square.cpp",
    "boss_square.h",
    "boss_super.cpp",
    "boss_super.h",
    "boss_tractor.cpp",
    "boss_tractor.h",
    "boss.cpp",
    "enemy.cpp",
    "enemy.h",
    "overmind.cpp",
    "player.cpp",
    "shape.cpp",
    "ship.cpp",
    "stars.cpp",
  ],
  deps = [":lib"],
)

cc_binary(
  name = "iispace",
  srcs = [
    "lib.cpp",
    "main.cpp",
    "replay.cpp",
    "save.cpp",
    "z0_game.cpp",
  ],
  deps = [
    ":game",
    ":iispace_proto_cc",
    ":lib",
    "//src/util:gamepad_header",
    "@ois",
    "@sfml",
  ],
)

cc_binary(
  name = "score",
  srcs = [
    "lib.cpp",
    "main.cpp",
    "replay.cpp",
    "save.cpp",
    "z0_game.cpp",
  ],
  defines = ["PLATFORM_SCORE"],
  deps = [
    ":game",
    ":iispace_proto_cc",
    ":lib",
    "//src/util:gamepad_header",
  ],
)