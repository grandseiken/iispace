cc_library(
  name = "target",
  hdrs = ["target.h"],
  deps = [
    "//game/common:math",
    "@glm",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "render",
  hdrs = ["gl_renderer.h"],
  srcs = [
    "gl_renderer.cc",
    "prepare_text.cc",
  ],
  deps = [
    ":target",
    "//game/common:math",
    "//game/common:types",
    "//game/common:ustring",
    "//game/render/data",
    "@glm",
  ],
  implementation_deps = [
    ":font_cache",
    ":noise_generator",
    ":shader_compiler",
    "//assets/fonts",
    "//game/io/font",
    "//game/render/gl",
    "//game/render/shaders",
    "@gl3w",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "shader_compiler",
  hdrs = ["shader_compiler.h"],
  srcs = ["shader_compiler.cc"],
  deps = [
    "//game/common:types",
    "//game/render/gl",
  ],
  implementation_deps = [
    "//game/render/shaders",
    "@psrdnoise//:shaders",
  ]
)

cc_library(
  name = "font_cache",
  hdrs = ["font_cache.h"],
  srcs = ["font_cache.cc"],
  deps = [
    ":target",
    "//game/common:math",
    "//game/common:types",
    "//game/common:ustring",
    "//game/io/font",
    "//game/render/data",
    "//game/render/gl",
    "@glm",
  ],
)

cc_library(
  name = "noise_generator",
  hdrs = ["noise_generator.h"],
  srcs = ["noise_generator.cc"],
  deps = [
    "//game/render/gl",
    "@glm",
  ],
  implementation_deps = ["@fastnoise2"],
)
