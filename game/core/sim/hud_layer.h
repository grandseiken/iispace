#ifndef II_GAME_CORE_SIM_HUD_LAYER_H
#define II_GAME_CORE_SIM_HUD_LAYER_H
#include "game/core/ui/game_stack.h"
#include "game/logic/sim/io/player.h"
#include <array>
#include <optional>
#include <string>

namespace ii {
struct render_output;
namespace ui {
class TextElement;
class Panel;
}  // namespace ui
class BarFill;
class PlayerHud;

// TODO: render boss health bar and magic shots, etc; better formatted HUD.
class HudLayer : public ui::GameLayer {
public:
  ~HudLayer() override = default;
  HudLayer(ui::GameStack& stack, game_mode mode);

  void set_data(const render_output& render) { render_ = &render; }
  void set_status_text(std::string s) { status_text_ = std::move(s); }
  void set_debug_text(std::uint32_t player_index, std::string s) {
    if (player_index < debug_text_.size()) {
      debug_text_[player_index] = std::move(s);
    }
  }

protected:
  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer&) const override;

private:
  game_mode mode_ = game_mode::kStandardRun;
  std::array<PlayerHud*, kMaxPlayers> huds_ = {nullptr};
  ui::Panel* bar_outer_ = nullptr;
  BarFill* bar_inner_ = nullptr;
  ui::TextElement* bar_text_ = nullptr;
  ui::TextElement* status_ = nullptr;
  std::string status_text_;
  std::array<std::string, kMaxPlayers> debug_text_;
  const render_output* render_ = nullptr;

  struct boss_bar_data {
    boss_bar_data() {}
    render::panel_style style = render::panel_style::kNone;
    ustring name;
    cvec4 colour{1.f};
    float value = 1.f;
    float smooth_value = 1.f;
    std::uint32_t timer = 0;
  };
  std::optional<boss_bar_data> boss_bar_;
};

}  // namespace ii

#endif
