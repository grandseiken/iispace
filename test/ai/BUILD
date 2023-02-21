load("//test:ai_test.bzl", "ai_test", "ai_netsim_test")

ai_test(players = 1, mode = "legacy_normal")
ai_test(players = 2, mode = "legacy_normal")
ai_test(players = 3, mode = "legacy_normal")
ai_test(players = 4, mode = "legacy_normal")
ai_test(players = 1, mode = "legacy_hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 2, mode = "legacy_hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 3, mode = "legacy_hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 4, mode = "legacy_hard", extra_args = ["--can_face_secret_boss"])

ai_netsim_test(players = 3, topology = "ABC", mode = "legacy_normal")
ai_netsim_test(players = 4, topology = "ABAB", mode = "legacy_hard")

ai_test(players = 1, mode = "standard_run")
ai_test(players = 2, mode = "standard_run")
ai_test(players = 3, mode = "standard_run")
ai_test(players = 4, mode = "standard_run")

# TODO: add score verification (that'll need to be updated when logic changes) in order to catch
# discrepancies across platforms/compilers.
ai_test(players = 2, mode = "standard_run", prefix = "fixed_seed", extra_args = ["--seed", "1337"])
ai_test(players = 4, mode = "standard_run", prefix = "fixed_seed", extra_args = ["--seed", "1337"])

ai_netsim_test(players = 3, topology = "ABC", mode = "standard_run")
ai_netsim_test(players = 4, topology = "ABAB", mode = "standard_run")
