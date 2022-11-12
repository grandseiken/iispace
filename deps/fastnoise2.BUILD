FASTSIMD_COPTS = select({
  "@bazel_tools//src/conditions:windows": [
    "/GL-",
    "/GS-",
    "/fp:fast",
  ],
  "//conditions:default": [
    "-ffast-math",
    "-fno-stack-protector",
  ],
}) + select({
  # Doesn't build with C++20 on MSVC.
  "@//:msvc_build": ["/std:c++17"],
  "//conditions:default": [],
})

cc_library(
  name = "fastnoise2",
  srcs = glob([
    "src/FastNoise/*.cpp",
    "src/FastNoise/*.h",
  ]),
  deps = [
    ":fastnoise_include",
    ":fastsimd",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "fastnoise_include",
  hdrs = glob([
    "include/FastNoise/**/*.h",
    "include/FastNoise/**/*.inl",
  ]),
  defines = ["FASTNOISE_STATIC_LIB"],
  includes = ["include"],
)

cc_library(
  name = "fastsimd",
  srcs = ["src/FastSIMD/FastSIMD.cpp"],
  deps = [
    ":fastsimd_include",
    ":fastsimd_level_avx2",
    ":fastsimd_level_avx512",
    ":fastsimd_level_scalar",
    ":fastsimd_level_sse2",
    ":fastsimd_level_sse3",
    ":fastsimd_level_ssse3",
    ":fastsimd_level_sse41",
    ":fastsimd_level_sse42",
  ],
  copts = FASTSIMD_COPTS,
)

cc_library(
  name = "fastsimd_level_avx2",
  srcs = ["src/FastSIMD/FastSIMD_Level_AVX2.cpp"],
  deps = [":fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": ["/arch:AVX2"],
    "//conditions:default": ["-mavx2", "-mfma"],
  })
)

cc_library(
  name = "fastsimd_level_avx512",
  srcs = ["src/FastSIMD/FastSIMD_Level_AVX512.cpp"],
  deps = [":fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": ["/arch:AVX512"],
    "//conditions:default": ["-mavx512f", "-mavx512dq", "-mfma"],
  })
)

cc_library(
  name = "fastsimd_level_scalar",
  srcs = ["src/FastSIMD/FastSIMD_Level_Scalar.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS,
)

cc_library(
  name = "fastsimd_level_sse2",
  srcs = ["src/FastSIMD/FastSIMD_Level_SSE2.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS,
)

cc_library(
  name = "fastsimd_level_sse3",
  srcs = ["src/FastSIMD/FastSIMD_Level_SSE3.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": [],
    "//conditions:default": ["-msse3"],
  }),
)

cc_library(
  name = "fastsimd_level_ssse3",
  srcs = ["src/FastSIMD/FastSIMD_Level_SSSE3.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": [],
    "//conditions:default": ["-mssse3"],
  }),
)

cc_library(
  name = "fastsimd_level_sse41",
  srcs = ["src/FastSIMD/FastSIMD_Level_SSE41.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": [],
    "//conditions:default": ["-msse4.1"],
  }),
)

cc_library(
  name = "fastsimd_level_sse42",
  srcs = ["src/FastSIMD/FastSIMD_Level_SSE42.cpp"],
  deps = ["fastsimd_internal"],
  copts = FASTSIMD_COPTS + select({
    "@//:msvc_build": [],
    "//conditions:default": ["-msse4.2"],
  }),
)

cc_library(
  name = "fastsimd_internal",
  hdrs = ["src/FastSIMD/FastSIMD_BuildList.inl"] + glob([
    "src/FastSIMD/Internal/*.h",
    "src/FastSIMD/Internal/*.inl",
  ]),
  deps = [":fastsimd_include"],
)

cc_library(
  name = "fastsimd_include",
  hdrs = glob(["include/FastSIMD/*.h"]),
  deps = [":fastnoise_include"],
  defines = ["FASTNOISE_STATIC_LIB"],
  includes = ["include"],
)
