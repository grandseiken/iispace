#ifndef IISPACE_GAME_RENDER_SHADERS_H
#define IISPACE_GAME_RENDER_SHADERS_H
#include <nonstd/span.hpp>
#include <cstdint>

namespace ii::shaders {

nonstd::span<const std::uint8_t> rect_fragment();
nonstd::span<const std::uint8_t> rect_vertex();

nonstd::span<const std::uint8_t> text_fragment();
nonstd::span<const std::uint8_t> text_vertex();

}  // namespace ii::shaders

#endif