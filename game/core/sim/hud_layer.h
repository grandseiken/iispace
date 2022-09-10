#ifndef II_GAME_CORE_SIM_HUD_LAYER_H
#define II_GAME_CORE_SIM_HUD_LAYER_H
#include "game/core/ui/game_stack.h"
#include <array>
#include <string>

namespace ii {
struct render_output;
namespace ui {
class TextElement;
}  // namespace ui

// TODO: render boss health bar and magic shots, etc; better formatted HUD.
class HudLayer : public ui::GameLayer {
public:
  ~HudLayer() override = default;
  HudLayer(ui::GameStack& stack, game_mode mode);

  void set_data(const render_output& render) { render_ = &render; }
  void set_status_text(const std::string& s) { status_text_ = s; }

protected:
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  game_mode mode_ = game_mode::kNormal;
  std::array<ui::TextElement*, 4> huds_ = {nullptr};
  ui::TextElement* status_ = nullptr;
  std::string status_text_;
  const render_output* render_ = nullptr;
};

}  // namespace ii

#endif