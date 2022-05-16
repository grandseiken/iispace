load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

################################################################################
# Skylib
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

################################################################################
# Dependencies
################################################################################

http_archive(
  name = "span_lite",
  build_file = "@//deps:span_lite.BUILD",
  sha256 = "3c139c67ecaa2704e057d242617c659d234bdc296eb5f863c193bd222e61cee6",
  strip_prefix = "span-lite-0.9.0",
  url = "https://github.com/martinmoene/span-lite/archive/v0.9.0.zip",
)

http_archive(
  name = "expected",
  build_file = "@//deps:expected.BUILD",
  sha256 = "c1733556cbd3b532a02b68e2fbc2091b5bc2cccc279e4f6c6bd83877aabd4b02",
  strip_prefix = "expected-1.0.0",
  url = "https://github.com/TartanLlama/expected/archive/v1.0.0.zip",
)

http_archive(
  name = "sdl_windows",
  build_file = "@//deps/sdl:sdl_windows.BUILD",
  sha256 = "32adc96d8b25e5671189f1f38a4fc7deb105fbb1b3ed78ffcb23f5b8f36b3922",
  strip_prefix = "SDL2-2.0.22",
  url = "https://www.libsdl.org/release/SDL2-devel-2.0.22-VC.zip"
)

http_archive(
  name = "gl3w",
  build_file = "@//deps:gl3w.BUILD",
  sha256 = "e96a650a5fb9530b69a19d36ef931801762ce9cf5b51cb607ee116b908a380a6",
  strip_prefix = "gl3w-5f8d7fd191ba22ff2b60c1106d7135bb9a335533",
  url = "https://github.com/skaslev/gl3w/archive/5f8d7fd191ba22ff2b60c1106d7135bb9a335533.zip",
)

http_archive(
  name = "sfml",
  build_file = "@//deps:sfml.BUILD",
  sha256 = "3e807f7e810d6357ede35acd97615f1fe67b17028ff3d3d946328afb6104ab86",
  strip_prefix = "sfml-2.5.1",
  url = "https://www.sfml-dev.org/files/SFML-2.5.1-windows-vc15-64-bit.zip",
)

http_archive(
  name = "ois",
  build_file = "@//deps:ois.BUILD",
  sha256 = "8997ff3d60ef49d4f1291a4100456ac6dddfa0b80eca2414d17c242501917e7e",
  strip_prefix = "OIS-1.5.1",
  url = "https://github.com/wgois/OIS/archive/refs/tags/v1.5.1.zip",
)

################################################################################
# Protobuf
################################################################################

http_archive(
  name = "rules_proto",
  sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
  strip_prefix = "rules_proto-4.0.0",
  urls = [
    "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
  ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()