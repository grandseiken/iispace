cc_library(
  name = "gl_renderer",
  hdrs = ["gl_renderer.h"],
  srcs = [
    "gl_renderer.cc",
    "shader_compiler.cc",
    "shader_compiler.h",
  ],
  deps = [
    "//assets/fonts",
    "//game/common:types",
    "//game/common:ustring",
    "//game/io/file:filesystem",
    "//game/io/font",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)