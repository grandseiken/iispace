# TODO: separate external and internal interfaces.
cc_library(
  name = "sim_interface",
  hdrs = [
    "sim_interface.h",
    "sim_io.h",
  ],
  deps = [
    "//game/common:math",
    "//game/mixer:sound",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sim",
  hdrs = ["sim_state.h"],
  srcs = [
    "random_engine.cc",
    "random_engine.h",
    "sim_interface.cc",
    "sim_internals.h",
    "sim_state.cc",
  ],
  deps = [
    ":sim_interface",
    "//game/logic",
    "//game/logic/ship",
  ],
  visibility = ["//visibility:public"],
)