load("//deps:xxd.bzl", "xxd")

xxd(
  name = "shaders_xxd",
  srcs = [
    "line.f.glsl",
    "line.v.glsl",
    "rect.f.glsl",
    "rect.v.glsl",
    "text.f.glsl",
    "text.v.glsl",
  ],
)

cc_library(
  name = "shaders",
  hdrs = [":shaders_xxd"],
  visibility = ["//visibility:public"],
)