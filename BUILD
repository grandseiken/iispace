# See https://github.com/hedronvision/bazel-compile-commands-extractor for setup.
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
CLANG_ARGS = "--config clang"
CLANG_CL_ARGS = "--config clang-cl"

refresh_compile_commands(
  name = "refresh_compile_commands_windows",
  targets = {"//game/...": CLANG_CL_ARGS},
)

refresh_compile_commands(
  name = "refresh_compile_commands_linux",
  targets = {"//game/...": CLANG_ARGS},
)
