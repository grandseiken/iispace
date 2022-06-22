# See https://github.com/hedronvision/bazel-compile-commands-extractor for setup.
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
CLANG_ARGS = "--config clang"

refresh_compile_commands(
  name = "refresh_compile_commands",
  targets = {
    "//game:iispace": CLANG_ARGS,
    "//game:score": CLANG_ARGS,
  },
)