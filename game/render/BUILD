cc_library(
  name = "gl_renderer",
  hdrs = ["gl_renderer.h"],
  srcs = [
    "gl_renderer.cc",
    "shader_compiler.cc",
    "shader_compiler.h",
  ],
  deps = [
    "//game/common:types",
    "//game/common:ustring",
    "//game/logic/sim/io:render",
    "@glm",
  ],
  implementation_deps = [
    "//assets/fonts",
    "//game/io/file:filesystem",
    "//game/io/font",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
  ],
  visibility = ["//visibility:public"],
)