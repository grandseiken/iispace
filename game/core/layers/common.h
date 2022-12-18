#ifndef II_GAME_CORE_LAYERS_COMMON_H
#define II_GAME_CORE_LAYERS_COMMON_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include <cstdint>

namespace ii {
// TODO: move out to common/colour or similar?
static constexpr ivec2 kUiDimensions = {640, 360};
static constexpr uvec2 kLargeFont = {14, 14};
static constexpr uvec2 kSemiLargeFont = {12, 12};
static constexpr uvec2 kMediumFont = {10, 10};
static constexpr uvec2 kSmallFont = {8, 8};
static constexpr uvec2 kSpacing = {8, 8};
static constexpr uvec2 kPadding = {4, 4};
static constexpr ivec2 kDropShadow = {2, 2};

static constexpr cvec4 kBackgroundColour = {1.f, 1.f, 1.f, 1.f / 4};
static constexpr cvec4 kTextColour = {1.f, 0.f, .65f, 1.f};
static constexpr cvec4 kErrorColour = {0.f, .65f, .5f, 1.f};
static constexpr cvec4 kHighlightColour = {1.f, 1.f, 1.f, 1.f};

namespace ui {
class Button;
class Element;
class LinearLayout;
}  // namespace ui

ui::Button& standard_button(ui::Button& button);
ui::LinearLayout& add_dialog_layout(ui::Element& element);
}  // namespace ii

#endif
