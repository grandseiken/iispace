load("//deps:xxd.bzl", "xxd")

xxd(
  name = "fonts_xxd",
  srcs = [
    "roboto_mono_bold.ttf",
    "roboto_mono_bold_italic.ttf",
    "roboto_mono_italic.ttf",
    "roboto_mono_regular.ttf",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "fonts",
  hdrs = [":fonts_xxd"],
  visibility = ["//visibility:public"],
)