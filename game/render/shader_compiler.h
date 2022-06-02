#ifndef II_GAME_RENDER_SHADER_COMPILER_H
#define II_GAME_RENDER_SHADER_COMPILER_H
#include "game/common/result.h"
#include "game/render/gl/program.h"
#include <string>
#include <unordered_map>

namespace ii::render {

result<gl::program>
compile_program(const std::unordered_map<std::string, gl::shader_type>& shaders);

}  // namespace ii::render

#endif