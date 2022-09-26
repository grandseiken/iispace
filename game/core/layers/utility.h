#ifndef II_GAME_CORE_LAYERS_UTILITY_H
#define II_GAME_CORE_LAYERS_UTILITY_H
#include "game/common/async.h"
#include "game/common/ustring.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/core/ui/game_stack.h"
#include <functional>
#include <type_traits>

namespace ii {
namespace detail {
template <typename T>
struct async_callback_type : std::type_identity<void(T)> {};
template <>
struct async_callback_type<void> : std::type_identity<void()> {};
}  // namespace detail

class ErrorLayer : public ui::GameLayer {
public:
  ~ErrorLayer() override = default;
  ErrorLayer(ui::GameStack& stack, ustring message, std::function<void()> on_close = {});
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  std::function<void()> on_close_;
};

template <typename T>
class AsyncWaitLayer : public ui::GameLayer {
public:
  using function_type = typename detail::async_callback_type<T>::type;

  ~AsyncWaitLayer() override = default;
  AsyncWaitLayer(ui::GameStack& stack, ustring message, async_result<T> async,
                 std::function<function_type> on_success = {})
  : ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate}
  , async_{std::move(async)}
  , on_success_{std::move(on_success)} {
    set_bounds(rect{kUiDimensions});

    ui::LinearLayout& layout = add_dialog_layout(*this);
    auto& title = *layout.add_back<ui::TextElement>();
    title.set_text(std::move(message))
        .set_colour(kTextColour)
        .set_font(render::font_id::kMonospaceItalic)
        .set_font_dimensions(kLargeFont);
  }

  void update_content(const ui::input_frame& input, ui::output_frame& output) override {
    ui::GameLayer::update_content(input, output);
    if (async_) {
      if (!*async_) {
        // TODO: causes slight visual glitch when both darken overlays are rendered at once
        // for one frame. Either fix by combining, or separate out newly-added stack layers
        // and elements so they don't appear until next frame.
        stack().template add<ErrorLayer>(ustring::utf8(async_->error()));
      } else {
        if (on_success_) {
          if constexpr (std::is_void_v<T>) {
            on_success_();
          } else {
            on_success_(std::move(**async_));
          }
        }
      }
      remove();
    }
  }

private:
  async_result<T> async_;
  std::function<function_type> on_success_;
};

}  // namespace ii

#endif
