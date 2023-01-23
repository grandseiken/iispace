load("//test:exe_test.bzl", "exe_test")

def ai_test(mode, players, runs=1, prefix="", extra_args=[], **kwargs):
  if runs > 1:
    extra_args = extra_args + ["--multithreaded"]
  exe_test(
    name = "%s%sp_%s" % (prefix + "_" if prefix else "", players, mode),
    bin = "//game/tools:ai_replay_synth",
    args = [
      "--verify",
      "--mode", mode,
      "--players", "%s" % players,
      "--runs", "%s" % runs,
    ] + extra_args,
    size = "medium",
    **kwargs,
  )

def ai_netsim_test(mode, players, topology = "", prefix="", extra_args=[], **kwargs):
  exe_test(
    name = "netsim_%s%sp_%s" % (prefix + "_" if prefix else "", players, mode),
    bin = "//game/tools:ai_network_sim",
    args = [
      "--mode", mode,
      "--players", "%s" % players,
      "--topology", "%s" % topology,
    ] + extra_args,
    size = "medium",
    **kwargs,
  )