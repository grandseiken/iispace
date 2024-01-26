load("@bazel_skylib//rules:expand_template.bzl", "expand_template")

expand_template(
  name = "samplerate_template",
  template = ":src/samplerate.c",
  out = ":src/samplerate_expanded.c",
  substitutions = {
    "PACKAGE": "\"libsamplerate\"",
    "VERSION": "\"0.2.2\"",
  },
)

cc_library(
  name = "libsamplerate",
  hdrs = ["include/samplerate.h"],
  srcs = [
    ":samplerate_template",
    "src/common.h",
    "src/fastest_coeffs.h",
    "src/high_qual_coeffs.h",
    "src/mid_qual_coeffs.h",
    "src/src_linear.c",
    "src/src_sinc.c",
    "src/src_zoh.c",
  ],
  includes = ["include"],
  copts = [
    "-Iexternal/_main~_repo_rules~libsamplerate/src",
    "-DHAVE_STDBOOL_H",
    "-DENABLE_SINC_FAST_CONVERTER",
    "-DENABLE_SINC_MEDIUM_CONVERTER",
    "-DENABLE_SINC_BEST_CONVERTER",
  ],
  visibility = ["//visibility:public"],
)