load("//deps:xxd.bzl", "xxd")

xxd(
  name = "shaders_xxd",
  srcs = [
    "legacy_line.f.glsl",
    "legacy_line.v.glsl",
    "legacy_rect.f.glsl",
    "legacy_rect.v.glsl",
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