filegroup(
  name = "sdl_windows_dll",
  srcs = ["lib/x64/SDL2.dll"],
  visibility = ["//visibility:public"],
)

cc_import(
  name = "sdl_windows_import",
  interface_library = "lib/x64/SDL2.lib",
  shared_library = "lib/x64/SDL2.dll",
)

cc_library(
  name = "sdl_windows",
  includes = ["include"],
  hdrs = glob(["include/*.h"]),
  deps = [":sdl_windows_import"],
  visibility = ["@//deps/sdl:__pkg__"],
)
