# Build config.
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

bool_flag(
  name = "steam",
  build_setting_default = False,
  visibility = ["//visibility:public"],
)

bool_flag(
  name = "msvc",
  build_setting_default = False,
  visibility = ["//visibility:public"],
)

config_setting(
  name = "steam_build",
  flag_values = {":steam": "true"},
)

config_setting(
  name = "msvc_build",
  flag_values = {":msvc": "true"},
)

# See https://github.com/hedronvision/bazel-compile-commands-extractor for setup.
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
LINUX_ARGS = "--config clang"
WINDOWS_ARGS = "--config clang-cl --steam"

refresh_compile_commands(
  name = "refresh_compile_commands_windows",
  targets = {"//game/...": WINDOWS_ARGS},
)

refresh_compile_commands(
  name = "refresh_compile_commands_linux",
  targets = {"//game/...": LINUX_ARGS},
)
