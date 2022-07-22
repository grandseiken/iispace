# See https://github.com/hedronvision/bazel-compile-commands-extractor for setup.
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
CLANG_CL_ARGS = "--config clang-cl"

refresh_compile_commands(
  name = "refresh_compile_commands",
  targets = {
    "//game:iispace": CLANG_CL_ARGS,
    "//game:score": CLANG_CL_ARGS,
  },
)
