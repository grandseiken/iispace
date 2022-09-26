#include "game/core/layers/main_menu.h"
#include "game/common/ustring_convert.h"
#include "game/core/game_options.h"
#include "game/core/layers/common.h"
#include "game/core/layers/run_lobby.h"
#include "game/core/layers/utility.h"
#include "game/core/sim/sim_layer.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"
#include "game/system/system.h"
#include <utility>

namespace ii {
namespace {

ui::LinearLayout& add_main_layout(ui::Element* e, bool align_end = true) {
  auto& panel = *e->add_back<ui::Panel>();
  panel.set_padding(kSpacing);
  auto& layout = *panel.add_back<ui::LinearLayout>();
  layout.set_wrap_focus(true).set_align_end(align_end).set_spacing(kPadding.y);
  return layout;
};

void add_button(ui::LinearLayout& layout, const char* text, std::function<void()> callback) {
  auto& button = *layout.add_back<ui::Button>();
  standard_button(button).set_callback(std::move(callback)).set_text(ustring::ascii(text));
  layout.set_absolute_size(button, kLargeFont.y + 2 * kPadding.y);
};

// TODO: maybe this should not be a separate layer, but currently a lot of UI edge-cases
// for UI switching is only handled for layers by the GameStack.
class CreateGameLayer : public ui::GameLayer {
public:
  CreateGameLayer(ui::GameStack& stack, bool host_online)
  : ui::GameLayer{stack, ui::layer_flag::kCaptureInput | ui::layer_flag::kCaptureRender}
  , host_online_{host_online} {
    set_bounds(rect{kUiDimensions});

    auto start_game = [this](game_mode mode) {
      initial_conditions conditions;
      conditions.compatibility = this->stack().options().compatibility;
      conditions.seed = static_cast<std::uint32_t>(time(0));
      conditions.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
      conditions.player_count = 0u;
      conditions.mode = mode;
      if (!host_online_) {
        this->stack().add<RunLobbyLayer>(conditions);
        return;
      }
      auto async = this->stack().system().create_lobby();
      this->stack().add<AsyncWaitLayer<void>>(
          ustring::ascii("Creating lobby..."), std::move(async),
          [this, conditions] { this->stack().add<RunLobbyLayer>(conditions); });
    };

    auto& layout = add_main_layout(this);
    add_button(layout, "Normal mode", [=] { start_game(game_mode::kNormal); });
    add_button(layout, "Boss mode", [=] { start_game(game_mode::kBoss); });
    add_button(layout, "Hard mode", [=] { start_game(game_mode::kHard); });
    add_button(layout, "Fast mode", [=] { start_game(game_mode::kFast); });
    add_button(layout, "W-hat mode", [=] { start_game(game_mode::kWhat); });
    add_button(layout, "Back", [=] { remove(); });

    stack.set_volume(.5f);
  }

  void update_content(const ui::input_frame& input, ui::output_frame& output) override {
    ui::GameLayer::update_content(input, output);
    if (input.pressed(ui::key::kCancel)) {
      output.sounds.emplace(sound::kMenuAccept);
      remove();
    }
  }

private:
  bool host_online_;
};

}  // namespace

MainMenuLayer::MainMenuLayer(ui::GameStack& stack) : ui::GameLayer{stack} {
  set_bounds(rect{kUiDimensions});

  auto& layout = add_main_layout(this);
  if (stack.system().supports_networked_multiplayer()) {
    add_button(layout, "Start local game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ false); });
    add_button(layout, "Host online game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ true); });
  } else {
    add_button(layout, "Start game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ false); });
  }
  add_button(layout, "Exit", [this] { exit_timer_ = 8; });

  auto& top = add_main_layout(this, false);
  top_text_ = top.add_back<ui::TextElement>();
  top_text_->set_font(render::font_id::kMonospace)
      .set_font_dimensions(kMediumFont)
      .set_alignment(ui::alignment::kTop | ui::alignment::kLeft)
      .set_colour(kTextColour)
      .set_multiline(true);

  stack.set_volume(.5f);
  focus();
}

void MainMenuLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  ui::GameLayer::update_content(input, output);
  stack().set_fps(60);

  auto& system = stack().system();
  if (system.supports_networked_multiplayer()) {
    std::size_t friends_online = 0;
    std::size_t friends_in_game = 0;
    for (const auto& info : system.friend_list()) {
      friends_online += info.in_game ? 1u : 0u;
      friends_in_game += info.lobby_id ? 1u : 0u;
    }
    auto text = "Logged in as: " + to_utf8(system.local_username()) +
        "\nFriends online: " + std::to_string(friends_online) +
        "\nFriends in-game: " + std::to_string(friends_in_game);
    top_text_->set_text(ustring::utf8(text));
  }

  if (exit_timer_) {
    if (!--exit_timer_) {
      remove();
    }
  }
}

}  // namespace ii
