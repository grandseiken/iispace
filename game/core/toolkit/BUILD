cc_library(
  name = "toolkit",
  hdrs = [
    "button.h",
    "layout.h",
    "panel.h",
    "text.h",
  ],
  srcs = [
    "button.cc",
    "layout.cc",
    "panel.cc",
    "text.cc",
  ],
  deps = [
    "//game/core/ui:element",
    "//game/common:types",
    "//game/common:ustring",
    "//game/render/data",
  ],
  implementation_deps = [
    "//game/core/ui:input",
    "//game/render",
  ],
  visibility = ["//visibility:public"],
)