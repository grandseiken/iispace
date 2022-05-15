load("@bazel_skylib//rules:expand_template.bzl", "expand_template")

expand_template(
  name = "ois_header",
  template = ":includes/OISPrereqs.h.in",
  out = ":includes/OISPrereqs.h",
  substitutions = {
    "@OIS_MAJOR_VERSION@": "1",
    "@OIS_MINOR_VERSION@": "5",
    "@OIS_PATCH_VERSION@": "1",
    "@OIS_SOVERSION@": "1.5.1",
  },
)

cc_library(
  name = "ois",
  includes = ["includes"],
  hdrs = [":includes/OISPrereqs.h"] + glob(["includes/*.h"]) + select({
    "@bazel_tools//src/conditions:windows": glob(["src/win32/*.h"]),
    "//conditions:default": glob(["src/linux/*.h"]),
  }),
  srcs = glob(["src/*.cpp"]) + select({
    "@bazel_tools//src/conditions:windows": glob(["src/win32/*.cpp"]),
    "//conditions:default": glob(["src/linux/*.cpp"]),
  }),
  visibility = ["//visibility:public"],
)