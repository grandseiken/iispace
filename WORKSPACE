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
# Bazel dependencies
################################################################################
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
  name = "bazel_skylib",
  sha256 = "dbbc4d58f9f3438bdb0986473e07ba19de28f7ac98d3d8a9cd841955542ffe5e",
  strip_prefix = "bazel-skylib-67bfa0ce4de5d4b512178d5f63abad1696f6c05b",
  urls = [
    "https://github.com/bazelbuild/bazel-skylib/archive/67bfa0ce4de5d4b512178d5f63abad1696f6c05b.zip",
  ],
)
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
    name = "rules_pkg",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
        "https://github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
    ],
    sha256 = "8a298e832762eda1830597d64fe7db58178aa84cd5926d76d5b744d6558941c2",
)
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()

################################################################################
# Custom repository rules
################################################################################
load("@//tools:http_archive_override.bzl", "http_archive_override")
load("@//tools:sdl_system_repository.bzl", "sdl_system_repository")

################################################################################
# Language feature dependencies
################################################################################
http_archive(
  # CC0 license, no attribution required.
  name = "expected",
  build_file = "@//deps:expected.BUILD",
  sha256 = "c1733556cbd3b532a02b68e2fbc2091b5bc2cccc279e4f6c6bd83877aabd4b02",
  strip_prefix = "expected-1.0.0",
  url = "https://github.com/TartanLlama/expected/archive/v1.0.0.zip",
)

http_archive(
  # MIT license, but I wrote it and can use it without attribution.
  name = "static_functional",
  sha256 = "d79740eb82f25e9728ee07f7344ac89e621125057e960b238b46976ebf8673d7",
  strip_prefix = "static-functional-df5154af7c66e8518494d16d88cdbd71698c1f29",
  url = "https://github.com/grandseiken/static-functional/archive/df5154af7c66e8518494d16d88cdbd71698c1f29.zip",
)

http_archive(
  # Apache license, attribution required?
  name = "gcem",
  build_file = "@//deps:gcem.BUILD",
  sha256 = "be312a6a3cf55e78fe7f8055c28fa14b1ea94282bd66841906388f1ef4d4698c",
  strip_prefix = "gcem-1.16.0",
  url = "https://github.com/kthohr/gcem/archive/refs/tags/v1.16.0.zip",
)

http_archive(
  # Boost license, no attribution required.
  name = "concurrent_queue",
  build_file = "@//deps:concurrent_queue.BUILD",
  sha256 = "5e9e229a1791e8299dcd4bd73ac1be1953424a903818feb7afd929eb16094ef5",
  strip_prefix = "concurrentqueue-1.0.3",
  url = "https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.3.zip",
)

################################################################################
# SDL
################################################################################
http_archive(
  # zlib license, no attribution required.
  name = "sdl_windows",
  build_file = "@//deps/sdl:sdl_windows.BUILD",
  sha256 = "f657026c51eda688b694b165ccfeebf031266c46a7a609d837e94cad3877ec26",
  strip_prefix = "SDL2-2.26.4",
  url = "https://github.com/libsdl-org/SDL/releases/download/release-2.26.4/SDL2-devel-2.26.4-VC.zip"
)

http_archive(
  # zlib license, no attribution required.
  name = "sdl_gamecontrollerdb",
  build_file = "@//deps/sdl:sdl_gamecontrollerdb.BUILD",
  sha256 = "e45bd267d5c4afef51eb2913c8ad97fb122aeda7eae13d06d10dc40ce6544ff8",
  strip_prefix = "SDL_GameControllerDB-47afeaa74afeef944f7f895bf67acbfa5d24621a",
  url = "https://github.com/gabomdq/SDL_GameControllerDB/archive/47afeaa74afeef944f7f895bf67acbfa5d24621a.zip",
)

sdl_system_repository(name = "sdl_system")

################################################################################
# OpenGL dependencies
################################################################################
http_archive(
  # MIT license, attribution required?
  name = "glm",
  build_file = "@//deps:glm.BUILD",
  sha256 = "4605259c22feadf35388c027f07b345ad3aa3b12631a5a316347f7566c6f1839",
  strip_prefix = "glm-0.9.9.8",
  url = "https://github.com/g-truc/glm/archive/0.9.9.8.zip",
)

http_archive(
  # Unlicense, no attribution required.
  name = "gl3w",
  build_file = "@//deps:gl3w.BUILD",
  sha256 = "e96a650a5fb9530b69a19d36ef931801762ce9cf5b51cb607ee116b908a380a6",
  strip_prefix = "gl3w-5f8d7fd191ba22ff2b60c1106d7135bb9a335533",
  url = "https://github.com/skaslev/gl3w/archive/5f8d7fd191ba22ff2b60c1106d7135bb9a335533.zip",
)

http_archive(
  # MIT license, attribution required?
  name = "fastnoise2",
  build_file = "@//deps:fastnoise2.BUILD",
  sha256 = "78706f4548557bcadeb8cc4aaed82dba6bb3188e34a1ce07c5df1f64da89be90",
  strip_prefix = "FastNoise2-11072bc0dbe4f41ea87839dfed369ddfe7c3864a",
  url = "https://github.com/Auburn/FastNoise2/archive/11072bc0dbe4f41ea87839dfed369ddfe7c3864a.zip",
)

http_archive(
  name = "psrdnoise",
  build_file = "@//deps:psrdnoise.BUILD",
  sha256 = "11a64c44d3ee24656e8d2e8e820e6161182dfe40522275abc9478e3a98c07fd8",
  strip_prefix = "psrdnoise-4d627ffbd2801db0243b6f62a56ea1e748fa4c5d",
  url = "https://github.com/stegu/psrdnoise/archive/4d627ffbd2801db0243b6f62a56ea1e748fa4c5d.zip",
)

################################################################################
# Media format dependencies
################################################################################
http_archive(
  # MIT no attribution or public domain, no attribution required.
  name = "dr_libs",
  build_file = "@//deps:dr_libs.BUILD",
  sha256 = "39ea8c1f9b60a945735dfe4a2e0a2a6bd3bc921619fa7d2612dbc284b68c2419",
  strip_prefix = "dr_libs-15f37e3ab01654c1a3bc98cff2a9ca64e8296fa9",
  url = "https://github.com/mackron/dr_libs/archive/15f37e3ab01654c1a3bc98cff2a9ca64e8296fa9.zip",
)

http_archive(
  # BSD licence, attribution required.
  name = "libsamplerate",
  build_file = "@//deps:libsamplerate.BUILD",
  sha256 = "7bd06fabd57027e9c0fa22e6bd873e4916ab96ea132d6cb1b76e7fdd2ade1d20",
  strip_prefix = "libsamplerate-0.2.2",
  url = "https://github.com/libsndfile/libsamplerate/archive/refs/tags/0.2.2.zip",
)

http_archive(
  # FTL license, attribution required.
  name = "freetype",
  build_file = "@//deps:freetype.BUILD",
  sha256 = "5dac723f3889d451fb0b50a1cde94373a99609a650684a595f334437189a601c",
  strip_prefix = "freetype-f9f6adb625c48ef15b5d61a3ac1709a068ea95a3",
  url = "https://github.com/freetype/freetype/archive/f9f6adb625c48ef15b5d61a3ac1709a068ea95a3.zip",
)

################################################################################
# Steamworks SDK
################################################################################
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

################################################################################
# Protobuf
################################################################################
http_archive(
  name = "rules_proto",
  sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
  strip_prefix = "rules_proto-5.3.0-21.7",
  urls = [
    "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
  ],
)

# libprotobuf has Google license, attribution required.
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

################################################################################
# Test utilities
################################################################################
http_archive(
  name = "flamegraph",
  build_file = "@//deps:flamegraph.BUILD",
  sha256 = "2ad11f0a446aab6089d1f2f0d8cd3b8cbde9d348e91c2c22716fc6a7650eac17",
  strip_prefix = "FlameGraph-810687f180f3c4929b5d965f54817a5218c9d89b",
  url = "https://github.com/brendangregg/FlameGraph/archive/810687f180f3c4929b5d965f54817a5218c9d89b.zip",
)
