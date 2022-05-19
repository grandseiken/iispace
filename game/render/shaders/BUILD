load("//deps:xxd.bzl", "xxd")

xxd(
  name = "shaders_xxd",
  srcs = [
    "rect.f.glsl",
    "rect.v.glsl",
    "text.f.glsl",
    "text.v.glsl",
  ],
)

cc_library(
  name = "shaders",
  hdrs = ["shaders.h"],
  srcs = [
    ":shaders_xxd",
    "shaders.cc",
  ],
  deps = ["@span_lite"],
  visibility = ["//visibility:public"],
)