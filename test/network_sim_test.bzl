def _network_sim_test_impl(ctx):
  ctx.actions.symlink(
    target_file = ctx.executable._replay_network_sim_bin,
    output = ctx.outputs.executable,
    is_executable = True,
  )

  return DefaultInfo(
    runfiles = ctx.runfiles(files = ctx.files.deps),
    executable = ctx.outputs.executable,
  )

_network_sim_test = rule(
  _network_sim_test_impl,
  attrs = {
    "deps": attr.label_list(allow_files = True),
    "_replay_network_sim_bin": attr.label(
      default = "//game:replay_network_sim",
      executable = True,
      cfg = "target",
    ),
  },
  test = True,
)

def network_sim_test(replay = "", topology = "", extra_args=[], **kwargs):
  _network_sim_test(
    name = "%s_%s" % (topology, Label(replay).name),
    deps = [replay],
    args = ["--topology", "%s" % topology, "$(location %s)" % replay] + extra_args,
    size = "small",
    **kwargs,
  )