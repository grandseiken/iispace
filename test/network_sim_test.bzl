load("//test:exe_test.bzl", "exe_test")

def network_sim_test(replay = "", topology = "", extra_args=[], **kwargs):
  exe_test(
    name = "%s_%s" % (topology, Label(replay).name),
    bin = "//game/tools:replay_network_sim",
    deps = [replay],
    args = ["--topology", "%s" % topology, "$(location %s)" % replay] + extra_args,
    size = "medium",
    **kwargs,
  )