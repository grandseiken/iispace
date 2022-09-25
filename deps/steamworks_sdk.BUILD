cc_import(
  name = "steamapi_linux",
  shared_library = "redistributable_bin/linux64/libsteam_api.so",
)

cc_import(
  name = "steamapi_darwin",
  shared_library = "redistributable_bin/osx/libsteam_api.dylib",
)

cc_import(
  name = "steamapi_windows",
  interface_library = "redistributable_bin/win64/steam_api64.lib",
  shared_library = "redistributable_bin/win64/steam_api64.dll",
)

filegroup(
  name = "steamapi_bin",
  srcs = select({
    "@bazel_tools//src/conditions:windows": [":redistributable_bin/win64/steam_api64.dll"],
    "@bazel_tools//src/conditions:linux_x86_64": [":redistributable_bin/linux64/libsteam_api.so"],
    "@bazel_tools//src/conditions:darwin_x86_64": [":redistributable_bin/osx/libsteam_api.dylib"],
  }),
  visibility = ["//visibility:public"],
)

cc_library(
  name = "steamworks_sdk",
  hdrs = glob(["public/steam/*.h"]),
  deps = select({
    "@bazel_tools//src/conditions:windows": [":steamapi_windows"],
    "@bazel_tools//src/conditions:linux_x86_64": [":steamapi_linux"],
    "@bazel_tools//src/conditions:darwin_x86_64": [":steamapi_darwin"],
  }),
  includes = ["public"],
  target_compatible_with = select({
    "@//:steam_build": [],
    "//conditions:default": ["@platforms//:incompatible"],
  }),
  visibility = ["//visibility:public"],
)

alias(
  name = "steamcmd",
  actual = select({
    "@bazel_tools//src/conditions:windows": ":tools/ContentBuilder/builder/steamcmd.exe",
    "@bazel_tools//src/conditions:linux_x86_64": ":tools/ContentBuilder/builder_linux/steamcmd.sh",
    "@bazel_tools//src/conditions:darwin_x86_64": ":tools/ContentBuilder/builder_osx/steamcmd.sh",
  }),
)