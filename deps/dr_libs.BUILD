load("@bazel_skylib//rules:write_file.bzl", "write_file")

write_file(
  name = "dr_wav_c",
  out = "dr_wav.c",
  content = ["#include <dr_wav.h>"],
)

cc_library(
  name = "dr_wav",
  hdrs = ["dr_wav.h"],
  srcs = ["dr_wav.c"],
  copts = ["-DDR_WAV_IMPLEMENTATION"],
  includes = ["."],
  visibility = ["//visibility:public"],
)