cc_library(
  name = "render_common",
  hdrs = ["render_common.h"],
  deps = [
    "//game/common:math",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "gl_renderer",
  hdrs = ["gl_renderer.h"],
  srcs = [
    "gl_renderer.cc",
    "shader_compiler.cc",
    "shader_compiler.h",
  ],
  deps = [
    ":render_common",
    "//game/common:math",
    "//game/common:types",
    "//game/common:ustring",
    "//game/logic/sim/io:render",
    "@glm",
  ],
  implementation_deps = [
    ":font_cache",
    "//assets/fonts",
    "//game/io/font",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "font_cache",
  hdrs = ["font_cache.h"],
  srcs = ["font_cache.cc"],
  deps = [
    ":render_common",
    "//game/common:math",
    "//game/common:types",
    "//game/common:ustring",
    "//game/io/font",
    "//game/render/gl",
    "@glm",
  ],
)
