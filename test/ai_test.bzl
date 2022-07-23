def _ai_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable._replay_synth_bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    executable = ctx.outputs.executable,
  )

_ai_test = rule(
  _ai_test_impl,
  attrs = {
    "_replay_synth_bin": attr.label(
      default = "//game:replay_synth",
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)

def ai_test(mode, players, runs=1, extra_args=[], **kwargs):
  _ai_test(
    name = "ai_test_%sp_%s" % (players, mode),
    args = [
      "--verify",
      "--mode", mode,
      "--players", "%s" % players,
      "--runs", "%s" % runs,
    ] + extra_args,
    size = "small",
    **kwargs,
  )