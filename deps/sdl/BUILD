cc_library(
  name = "sdl",
  deps = select({
    "@bazel_tools//src/conditions:windows": ["@sdl_windows//:sdl_windows"],
  }),
  visibility = ["//visibility:public"],
)
