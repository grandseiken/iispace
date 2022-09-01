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
