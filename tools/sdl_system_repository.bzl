def _sdl2_config(ctx, args):
  result = ctx.execute(["sdl2-config"] + args)
  if result.return_code > 0:
    fail("sdl2-config returned " + str(result.return_code) + ":\n", result.stderr)
  if result.stderr:
    print("sdl2-config stderr:\n", result.stderr)
  return result.stdout.strip()

def _sdl_system_repository_impl(ctx):
  prefix = _sdl2_config(ctx, ["--prefix"])
  libs = _sdl2_config(ctx, ["--libs"])

  linkopts = "[%s]" % ", ".join(['"%s"' % arg for arg in libs.split(" ")])

  ctx.symlink("%s/include/SDL2" % prefix, "include")
  ctx.template(
    "BUILD",
    ctx.attr._build_file_template,
    substitutions = {
      "%NAME%": ctx.attr.name,
      "%LINKOPTS%": linkopts,
    },
    executable = False,
  )

sdl_system_repository = repository_rule(
  implementation=_sdl_system_repository_impl,
  attrs = {"_build_file_template": attr.label(default = "@//deps/sdl:sdl_system.BUILD")},
  local=True,
)
