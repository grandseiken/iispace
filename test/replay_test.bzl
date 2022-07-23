def _replay_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable._score_bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    runfiles = ctx.runfiles(files = ctx.files.deps),
    executable = ctx.outputs.executable,
  )

_replay_test = rule(
  _replay_test_impl,
  attrs = {
    "deps": attr.label_list(allow_files = True),
    "_score_bin": attr.label(
      default = "//game:score",
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)

def replay_test(replay = "", score = 0, **kwargs):
  _replay_test(
    name = "test_%s" % replay,
    deps = [":%s" % replay],
    args = ["--verify_score", "%s" % score, "$(location :%s)" % replay],
    size = "small",
    **kwargs,
  )