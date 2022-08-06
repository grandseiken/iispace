load("//test:network_sim_test.bzl", "network_sim_test")

HIGH_VARIANCE_ARGS = [
  "--max_tick_difference=64",
  "--max_tick_delivery_delay=16",
]

LOW_VARIANCE_ARGS = [
  "--max_tick_difference=16",
  "--max_tick_delivery_delay=4",
]

MIN_VARIANCE_ARGS = [
  "--max_tick_difference=8",
  "--max_tick_delivery_delay=2",
]

network_sim_test(replay = "//test/replays:Darb_2p__Graves__Darb_553403.wrp",
                 topology = "AB", extra_args = MIN_VARIANCE_ARGS)
network_sim_test(replay = "//test/replays:Darb_4p__Team_Graves_430987.wrp",
                 topology = "ABAB", extra_args = LOW_VARIANCE_ARGS)
network_sim_test(replay = "//test/replays:seiken_2p__RAB__STU_Yo_477833.wrp",
                 topology = "AB", extra_args = LOW_VARIANCE_ARGS)
network_sim_test(replay = "//test/replays:seiken_3p__3_OF_US_219110.wrp",
                 topology = "ABC", extra_args = HIGH_VARIANCE_ARGS)