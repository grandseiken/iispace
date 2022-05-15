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
  sha256 = "bf1e0643acb92369b24572b703473af60bac82caf5af61e77c063b779471bb7f",
  strip_prefix = "sfml-2.5.1",
  url = "https://github.com/SFML/SFML/releases/download/2.5.1/SFML-2.5.1-sources.zip",
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
  sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
  strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
  urls = [
    "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
    "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
  ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()