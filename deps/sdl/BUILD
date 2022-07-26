cc_library(
  name = "sdl",
  deps = select({
    "@bazel_tools//src/conditions:windows": ["@sdl_windows"],
    "@bazel_tools//src/conditions:linux_x86_64": ["@sdl_system"],
  }),
  visibility = ["//visibility:public"],
)
