#ifndef II_GAME_CORE_LAYERS_COMMON_H
#define II_GAME_CORE_LAYERS_COMMON_H
#include <glm/glm.hpp>
#include <cstdint>

namespace ii {
static constexpr glm::ivec2 kUiDimensions = {640, 360};
static constexpr glm::uvec2 kLargeFont = {16, 16};
static constexpr glm::uvec2 kMediumFont = {10, 10};
static constexpr glm::uvec2 kSpacing = {16, 16};
static constexpr glm::uvec2 kPadding = {4, 4};
static constexpr glm::ivec2 kDropShadow = {2, 2};
}  // namespace ii

#endif
