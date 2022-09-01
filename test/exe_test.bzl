def _exe_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable.bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    runfiles = ctx.runfiles(files = ctx.files.deps),
    executable = ctx.outputs.executable,
  )

exe_test = rule(
  _exe_test_impl,
  attrs = {
    "deps": attr.label_list(allow_files = True),
    "bin": attr.label(
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)