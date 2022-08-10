load("@rules_pkg//:mappings.bzl", "pkg_files")

filegroup(
  name = "sounds",
  srcs = glob(["*.wav"]),
)

pkg_files(
  name = "assets",
  srcs = [":sounds"],
  prefix = "assets",
  visibility = ["//visibility:public"],
)
