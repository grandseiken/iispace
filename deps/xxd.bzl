load("@bazel_skylib//rules:write_file.bzl", "write_file")

def _sanitize(s):
  return s.replace("/", "_").replace(".", "_").replace(" ", "_").replace("-", "_")

def xxd(name, namespace, srcs, testonly = False, visibility = None):
  for src in srcs:
    native.genrule(
      name = "%s_xxd" % src,
      srcs = [src],
      outs = ["%s.h" % src],
      cmd = "xxd -i \"$<\" \"$@\"",
      testonly = testonly,
    )

  repository = native.repository_name().replace("@", "")
  rdir = ["external", repository] if repository else []
  pdir = native.package_name().split("/") if native.package_name() else []

  guard = "_".join(rdir + pdir + [name, "H"])
  h_content = [
    "#ifndef %s" % guard,
    "#define %s" % guard,
    "#include <cstdint>",
    "#include <span>",
    "#include <string>",
    "#include <unordered_map>",
    "namespace %s {" % namespace,
    "std::unordered_map<std::string, std::span<const std::uint8_t>> %s_filemap();" % name,
  ]

  cc_content = ["#include \"%s.h\"" % "/".join(pdir + [name])] + [
    "#include \"%s.h\"" % "/".join(pdir + [src]) for src in srcs
  ] + ["namespace %s {" % namespace]

  for src in srcs:
    src_name = _sanitize(src)
    var_name = "_".join(rdir + pdir + [src_name])
    h_content += ["std::span<const std::uint8_t> %s();" % src_name]
    cc_content += [
      "std::span<const std::uint8_t> %s() {" % src_name,
      "  return {::%s, ::%s_len};" % (var_name, var_name),
      "}",
    ]

  cc_content += [
    "std::unordered_map<std::string, std::span<const std::uint8_t>> %s_filemap() {" % name,
    "  static const std::unordered_map<std::string, std::span<const std::uint8_t>> kFiles = {",
  ]
  for src in srcs:
    cc_content += ["    {\"%s\", %s()}," % ("/".join(rdir + pdir + [src]), _sanitize(src))]
  cc_content += [
    "  };",
    "  return kFiles;",
    "}",
  ]

  h_content += ["}", "#endif"]
  cc_content += ["}"]

  write_file(name = "%s_h" % name, out = "%s.h" % name, content = h_content)
  write_file(name = "%s_c" % name, out = "%s.cc" % name, content = cc_content)

  native.cc_library(
    name = name,
    hdrs = ["%s.h" % name],
    srcs = ["%s.cc" % name] + ["%s.h" % src for src in srcs],
    testonly = testonly,
    visibility = visibility,
  )

def xxd_legacy(name, srcs, testonly = False, visibility = None):
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
