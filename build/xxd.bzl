def xxd(name, srcs, testonly = False, visibility = None):
  for src in srcs:
    native.genrule(
      name = "%s_xxd" % src,
      srcs = [src],
      outs = ["%s.h" % src],
      cmd = "xxd -i \"$<\" \"$@\"",
      testonly = testonly,
    )

  native.filegroup(
    name = name,
    srcs = ["%s.h" % src for src in srcs],
    testonly = testonly,
    visibility = visibility,
  )
