load("@bazel_skylib//lib:paths.bzl", "paths")

def _http_archive_override_impl(ctx):
  urls = []

  root_dir = ctx.path(Label("//:WORKSPACE")).dirname
  if paths.is_absolute(ctx.attr.local_path):
    urls += ["file:///%s" % ctx.attr.local_path]
  else:
    urls += ["file:///%s/%s" % (root_dir, ctx.attr.local_path)]

  if ctx.attr.url:
    urls += [ctx.attr.url]
  if ctx.attr.urls:
    urls += ctx.attr.urls

  ctx.download_and_extract(
    sha256 = ctx.attr.sha256,
    stripPrefix = ctx.attr.strip_prefix,
    url = urls,
  )
  ctx.symlink(ctx.attr.build_file, "BUILD.bazel")

http_archive_override = repository_rule(
  implementation=_http_archive_override_impl,
  attrs = {
    "build_file": attr.label(),
    "sha256": attr.string(),
    "strip_prefix": attr.string(),
    "local_path": attr.string(),
    "url": attr.string(),
    "urls": attr.string_list(),
  },
  local = True,
)
