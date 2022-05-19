load("@//deps:xxd.bzl", "xxd")

xxd(
  name = "sdl_gamecontrollerdb_xxd",
  srcs = ["gamecontrollerdb.txt"],
)

cc_library(
  name = "sdl_gamecontrollerdb",
  hdrs = [":sdl_gamecontrollerdb_xxd"],
  visibility = ["//visibility:public"],
)