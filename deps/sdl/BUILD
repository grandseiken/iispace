cc_library(
  name = "sdl",
  deps = select({
    "@bazel_tools//src/conditions:windows": ["@sdl_windows"],
    "//conditions:default": ["@sdl_system"],
  }),
  visibility = ["//visibility:public"],
)
