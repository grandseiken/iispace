cc_import(
  name = "flac",
  static_library = "lib/flac.lib",
)

cc_import(
  name = "freetype",
  static_library = "lib/freetype.lib",
)

cc_import(
  name = "ogg",
  static_library = "lib/ogg.lib",
)

cc_import(
  name = "openal",
  interface_library = "lib/openal32.lib",
  shared_library = "bin/openal32.dll",
)

cc_import(
  name = "sfml_audio",
  static_library = "lib/sfml-audio-s.lib",
)

cc_import(
  name = "sfml_graphics",
  static_library = "lib/sfml-graphics-s.lib",
)

cc_import(
  name = "sfml_system",
  static_library = "lib/sfml-system-s.lib",
)

cc_import(
  name = "sfml_window",
  static_library = "lib/sfml-window-s.lib",
)

cc_import(
  name = "vorbis",
  static_library = "lib/vorbis.lib",
)

cc_import(
  name = "vorbisenc",
  static_library = "lib/vorbisenc.lib",
)

cc_import(
  name = "vorbisfile",
  static_library = "lib/vorbisfile.lib",
)

cc_library(
  name = "sfml",
  includes = ["include"],
  hdrs = glob(["include/SFML/**/*.hpp"]),
  deps = [
    ":flac",
    ":freetype",
    ":ogg",
    ":openal",
    ":sfml_audio",
    ":sfml_graphics",
    ":sfml_system",
    ":sfml_window",
    ":vorbis",
    ":vorbisenc",
    ":vorbisfile",
  ],
  defines = ["SFML_STATIC"],
  visibility = ["//visibility:public"],
)