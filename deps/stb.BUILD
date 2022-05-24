load("@bazel_skylib//rules:write_file.bzl", "write_file")

write_file(
  name = "stb_image_c",
  out = "stb_image.c",
  content = ["#include <stb_image.h>"],
)

cc_library(
  name = "stb_image",
  hdrs = ["stb_image.h"],
  srcs = ["stb_image.c"],
  copts = ["-DSTB_IMAGE_IMPLEMENTATION"],
  includes = ["."],
  visibility = ["//visibility:public"],
)