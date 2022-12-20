cc_library(
  name = "mixer",
  hdrs = ["mixer.h"],
  srcs = ["mixer.cc"],
  deps = ["//game/common:types"],
  implementation_deps = [
    "//game/common:math",
    "@dr_libs//:dr_wav",
    "@libsamplerate",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "sound",
  hdrs = ["sound.h"],
  visibility = ["//visibility:public"],
)