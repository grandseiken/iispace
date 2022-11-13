load("@//tools:xxd.bzl", "xxd")

xxd(
  name = "shaders",
  namespace = "ii::render::psrdnoise_shaders",
  srcs = glob(["src/*.glsl"]),
  visibility = ["@//game/render:__pkg__"],
)