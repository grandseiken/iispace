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