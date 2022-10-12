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