#ifndef II_GAME_CORE_LAYERS_COMMON_H
#define II_GAME_CORE_LAYERS_COMMON_H
#include <glm/glm.hpp>
#include <cstdint>

namespace ii {
static constexpr glm::ivec2 kUiDimensions = {640, 360};
static constexpr glm::uvec2 kLargeFont = {16, 16};
static constexpr glm::uvec2 kSemiLargeFont = {12, 12};
static constexpr glm::uvec2 kMediumFont = {10, 10};
static constexpr glm::uvec2 kSpacing = {16, 16};
static constexpr glm::uvec2 kPadding = {4, 4};
static constexpr glm::ivec2 kDropShadow = {2, 2};

static constexpr glm::vec4 kBackgroundColour = {1.f, 1.f, 1.f, 1.f / 8};
static constexpr glm::vec4 kTextColour = {1.f, 0.f, .65f, 1.f};
static constexpr glm::vec4 kErrorColour = {0.f, .65f, .5f, 1.f};
static constexpr glm::vec4 kHighlightColour = {1.f, 1.f, 1.f, 1.f};

namespace ui {
class Button;
class Element;
class LinearLayout;
}  // namespace ui

ui::Button& standard_button(ui::Button& button);
ui::LinearLayout& add_dialog_layout(ui::Element& element);
}  // namespace ii

#endif
