load("//test:exe_test.bzl", "exe_test")

def replay_test(replay = "", score = 0, **kwargs):
  exe_test(
    name = Label(replay).name,
    bin = "//game/tools:replay",
    deps = [replay],
    args = ["--verify_score", "%s" % score, "$(location %s)" % replay],
    size = "medium",
    **kwargs,
  )