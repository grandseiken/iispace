cc_library(
  name = "replay_tools",
  hdrs = ["replay_tools.h"],
  deps = [
    "//game/common:types",
    "//game/common:printer",
    "//game/data:replay",
    "//game/logic/sim",
    "//game/logic/sim/io:conditions",
    "//game/logic/sim/io:output",
  ]
)

cc_library(
  name = "network_tools",
  hdrs = ["network_tools.h"],
  deps = [
    "//game:flags",
    "//game/common:types",
    "//game/data:packet",
    "//game/data:replay",
    "//game/logic/sim:networked_sim_state",
    "//game/logic/sim/io:aggregate",
    "@concurrent_queue",
  ],
)

cc_binary(
  name = "replay",
  srcs = ["replay.cc"],
  deps = [
    ":replay_tools",
    "//game:flags",
    "//game/data:replay",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "replay_network_sim",
  srcs = ["replay_network_sim.cc"],
  deps = [
    ":network_tools",
    ":replay_tools",
    "//game:flags",
    "//game/common:types",
    "//game/data:replay",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "ai_replay_synth",
  srcs = ["ai_replay_synth.cc"],
  deps = [
    ":replay_tools",
    "//game:flags",
    "//game:mode_flags",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "ai_network_sim",
  srcs = ["ai_network_sim.cc"],
  deps = [
    ":network_tools",
    ":replay_tools",
    "//game:flags",
    "//game:mode_flags",
    "//game/io/file:std_filesystem",
  ],
  visibility = ["//visibility:public"],
)