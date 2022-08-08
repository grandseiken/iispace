load("//test:ai_test.bzl", "ai_test", "ai_netsim_test")

ai_test(players = 1, mode = "normal")
ai_test(players = 2, mode = "normal")
ai_test(players = 3, mode = "normal")
ai_test(players = 4, mode = "normal")
ai_test(players = 1, mode = "hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 2, mode = "hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 3, mode = "hard", extra_args = ["--can_face_secret_boss"])
ai_test(players = 4, mode = "hard", extra_args = ["--can_face_secret_boss"])

ai_netsim_test(players = 3, topology = "ABC", mode = "normal")
ai_netsim_test(players = 4, topology = "ABAB", mode = "hard")