#include "game/render/shader_compiler.h"
#include "game/render/shaders/shaders.h"
#include <unordered_set>

namespace ii::render {
namespace {

result<std::string> preprocess(const std::string& filename) {
  const auto& filemap = shaders::shaders_filemap();
  auto it = filemap.find(filename);
  if (it == filemap.end()) {
    return unexpected("Shader not found: " + filename);
  }
  std::string source{reinterpret_cast<const char*>(it->second.data()), it->second.size()};

  std::unordered_set<std::string> included_set{filename};
  static const std::string kInclude = "#include";
  while (true) {
    auto pos = source.find("#include");
    if (pos == std::string::npos) {
      break;
    }
    auto end = source.find_first_of('\n', pos);
    auto rest = source.substr(pos + kInclude.size(), end - pos - kInclude.size());
    auto lq = rest.find_first_of('"');
    auto rq = rest.find_last_of('"');
    if (lq == std::string::npos || rq == std::string::npos || lq >= rq) {
      return unexpected("Error preprocessing " + filename + ": invalid include");
    }
    auto path = rest.substr(lq + 1, rq - lq - 1);
    if (included_set.contains(path)) {
      source = source.substr(0, pos) + source.substr(end);
      continue;
    }
    included_set.emplace(path);
    auto it = filemap.find(path);
    if (it == filemap.end()) {
      return unexpected("Shader not found: " + path + " (included by " + filename + ")");
    }
    source = source.substr(0, pos) +
        std::string{reinterpret_cast<const char*>(it->second.data()), it->second.size()} +
        source.substr(end);
  }
  return source;
}

}  // namespace

result<gl::program>
compile_program(const std::unordered_map<std::string, gl::shader_type>& shaders) {
  std::vector<gl::shader> compiled_shaders;
  for (const auto& pair : shaders) {
    auto source = preprocess(pair.first);
    if (!source) {
      return unexpected(source.error());
    }
    auto result = gl::compile_shader(
        pair.second, {reinterpret_cast<const std::uint8_t*>(source->data()), source->size()});
    if (!result) {
      return unexpected("Error compiling " + pair.first + ": " + result.error());
    }
    compiled_shaders.emplace_back(std::move(*result));
  }
  auto result = gl::link_program(compiled_shaders);
  if (!result) {
    std::string names;
    for (const auto& pair : shaders) {
      if (!names.empty()) {
        names += ", ";
      }
      names += pair.first;
    }
    return unexpected("Error linking " + names + ": " + result.error());
  }
  return {std::move(*result)};
}

}  // namespace ii::render
