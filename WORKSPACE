load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

################################################################################
# Hedron compile commands extractor
################################################################################
http_archive(
    name = "hedron_compile_commands",
    sha256 = "2e72c14ec0578ce07a3b76965ed6b67894179df431d735df15beb8ec0371c6d6",
    strip_prefix = "bazel-compile-commands-extractor-749cd084143d5b5f52857c9f8e3442df58b7ca5f",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/749cd084143d5b5f52857c9f8e3442df58b7ca5f.zip",
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()

################################################################################
# SDL
################################################################################
http_archive(
  name = "sdl_windows",
  build_file = "@//deps/sdl:sdl_windows.BUILD",
  sha256 = "70aaa57cf918d85833665f8f84fbdfb8df0a0a619c764c780846a4b1851176b3",
  strip_prefix = "SDL2-2.28.0",
  url = "https://github.com/libsdl-org/SDL/releases/download/release-2.28.0/SDL2-devel-2.28.0-VC.zip"
)

http_archive(
  name = "sdl_gamecontrollerdb",
  build_file = "@//deps/sdl:sdl_gamecontrollerdb.BUILD",
  sha256 = "e45bd267d5c4afef51eb2913c8ad97fb122aeda7eae13d06d10dc40ce6544ff8",
  strip_prefix = "SDL_GameControllerDB-47afeaa74afeef944f7f895bf67acbfa5d24621a",
  url = "https://github.com/gabomdq/SDL_GameControllerDB/archive/47afeaa74afeef944f7f895bf67acbfa5d24621a.zip",
)

load("@//tools:sdl_system_repository.bzl", "sdl_system_repository")
sdl_system_repository(name = "sdl_system")

################################################################################
# OpenGL dependencies
################################################################################
http_archive(
  name = "glm",
  build_file = "@//deps:glm.BUILD",
  sha256 = "4605259c22feadf35388c027f07b345ad3aa3b12631a5a316347f7566c6f1839",
  strip_prefix = "glm-0.9.9.8",
  url = "https://github.com/g-truc/glm/archive/0.9.9.8.zip",
)

http_archive(
  name = "gl3w",
  build_file = "@//deps:gl3w.BUILD",
  sha256 = "e96a650a5fb9530b69a19d36ef931801762ce9cf5b51cb607ee116b908a380a6",
  strip_prefix = "gl3w-5f8d7fd191ba22ff2b60c1106d7135bb9a335533",
  url = "https://github.com/skaslev/gl3w/archive/5f8d7fd191ba22ff2b60c1106d7135bb9a335533.zip",
)

http_archive(
  name = "fastnoise2",
  build_file = "@//deps:fastnoise2.BUILD",
  integrity = "sha256-/567IbUjn1PKh1qR/o6EjTzbqg92cYJ7qJdJ7nyKzAY=",
  strip_prefix = "FastNoise2-d4a1a5ec208898985be9255a573fb03135cf8530",
  url = "https://github.com/Auburn/FastNoise2/archive/d4a1a5ec208898985be9255a573fb03135cf8530.zip",
)

http_archive(
  name = "psrdnoise",
  build_file = "@//deps:psrdnoise.BUILD",
  sha256 = "11a64c44d3ee24656e8d2e8e820e6161182dfe40522275abc9478e3a98c07fd8",
  strip_prefix = "psrdnoise-4d627ffbd2801db0243b6f62a56ea1e748fa4c5d",
  url = "https://github.com/stegu/psrdnoise/archive/4d627ffbd2801db0243b6f62a56ea1e748fa4c5d.zip",
)

################################################################################
# Steamworks SDK
################################################################################
load("@//tools:http_archive_override.bzl", "http_archive_override")
STEAMWORKS_SDK_VERSION = 157
http_archive_override(
  name = "steamworks_sdk",
  build_file = "@//deps:steamworks_sdk.BUILD",
  sha256 = "3d5ab5d2b5538fdbe49fd81abf3b6bc6c18b91bcc6a0fecd4122f22b243ee704",
  strip_prefix = "sdk",
  # Copy steamworks zip into repo root to source locally.
  local_path = "steamworks_sdk_%s.zip" % STEAMWORKS_SDK_VERSION,
  urls = [
    "https://partner.steamgames.com/downloads/steamworks_sdk_%s.zip" % STEAMWORKS_SDK_VERSION,
  ],
)
