cc_library(
  name = "gl_renderer",
  hdrs = ["gl_renderer.h"],
  srcs = ["gl_renderer.cc"],
  deps = [
    "//game/common:types",
    "//game/io/file:filesystem",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
    "@glm",
    "@stb//:stb_image",
  ],
  visibility = ["//visibility:public"],
)