cc_library(
  name = "renderer",
  hdrs = ["renderer.h"],
  deps = [
    "//game/common:types",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "gl_renderer",
  hdrs = ["gl_renderer.h"],
  srcs = ["gl_renderer.cc"],
  deps = [
    ":renderer",
    "//game/common:types",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
    "@glm",
    "@lodepng",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "null_renderer",
  hdrs = ["null_renderer.h"],
  deps = [":renderer"],
  visibility = ["//visibility:public"],
)