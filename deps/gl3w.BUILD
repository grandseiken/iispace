py_binary(
  name = "gl3w_gen",
  srcs = ["gl3w_gen.py"],
)

genrule(
  name = "gl3w_srcs",
  tools = [":gl3w_gen"],
  cmd = "./$(location :gl3w_gen) --root \"$(RULEDIR)\"",
  outs = [
    "include/GL/gl3w.h",
    "include/GL/glcorearb.h",
    "include/KHR/khrplatform.h",
    "src/gl3w.c",
  ],
)

cc_library(
  name = "gl3w",
  includes = ["include"],
  hdrs = [
    ":include/GL/gl3w.h",
    ":include/GL/glcorearb.h",
    ":include/KHR/khrplatform.h",
  ],
  srcs = [":src/gl3w.c"],
  visibility = ["//visibility:public"],
)
