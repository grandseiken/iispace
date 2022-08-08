cc_import(
  name = "steamapi_linux",
  shared_library = "redistributable_bin/linux64/libsteam_api.so",
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
  }),
  includes = ["public"],
  visibility = ["//visibility:public"],
)