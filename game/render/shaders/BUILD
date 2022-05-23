load("//deps:xxd.bzl", "xxd")

xxd(
  name = "shaders_xxd",
  srcs = [
    "legacy_text.f.glsl",
    "legacy_text.v.glsl",
    "rect.f.glsl",
    "rect.v.glsl",
    "text.f.glsl",
    "text.v.glsl",
  ],
)

cc_library(
  name = "shaders",
  hdrs = [":shaders_xxd"],
  deps = ["@span_lite"],
  visibility = ["//visibility:public"],
)