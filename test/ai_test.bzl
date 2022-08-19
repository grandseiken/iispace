def _ai_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable._ai_replay_synth_bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    executable = ctx.outputs.executable,
  )

_ai_test = rule(
  _ai_test_impl,
  attrs = {
    "_ai_replay_synth_bin": attr.label(
      default = "//game:ai_replay_synth",
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)

def ai_test(mode, players, runs=1, extra_args=[], **kwargs):
  if runs > 1:
    extra_args = extra_args + ["--multithreaded"]
  _ai_test(
    name = "%sp_%s" % (players, mode),
    args = [
      "--verify",
      "--mode", mode,
      "--players", "%s" % players,
      "--runs", "%s" % runs,
    ] + extra_args,
    size = "medium",
    **kwargs,
  )

def _ai_netsim_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable._ai_network_sim_bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    executable = ctx.outputs.executable,
  )

_ai_netsim_test = rule(
  _ai_netsim_test_impl,
  attrs = {
    "_ai_network_sim_bin": attr.label(
      default = "//game:ai_network_sim",
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)

def ai_netsim_test(mode, players, topology = "", extra_args=[], **kwargs):
  _ai_netsim_test(
    name = "netsim_%sp_%s" % (players, mode),
    args = [
      "--mode", mode,
      "--players", "%s" % players,
      "--topology", "%s" % topology,
    ] + extra_args,
    size = "medium",
    **kwargs,
  )