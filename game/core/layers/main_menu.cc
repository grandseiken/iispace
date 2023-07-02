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
#include "game/version.h"
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
    set_bounds(irect{kUiDimensions});

    auto start_game = [this](game_mode mode) {
      initial_conditions conditions;
      conditions.compatibility = this->stack().options().compatibility;
      conditions.seed = static_cast<std::uint32_t>(time(nullptr));
      conditions.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
      conditions.player_count = 0u;
      conditions.mode = mode;
      if (conditions.mode == game_mode::kStandardRun) {
        conditions.biomes.emplace_back(run_biome::kBiome0_Uplink);
        conditions.biomes.emplace_back(run_biome::kBiome1_Edge);
        conditions.biomes.emplace_back(run_biome::kBiome2_Fabric);
      }
      if (!host_online_) {
        this->stack().add<RunLobbyLayer>(conditions, /* online */ false);
        return;
      }
      auto async = this->stack().system().create_lobby();
      this->stack().add<AsyncWaitLayer<void>>(
          ustring::ascii("Creating lobby..."), std::move(async),
          [this, conditions] { this->stack().add<RunLobbyLayer>(conditions, /* online */ true); });
    };

    auto& layout = add_main_layout(this);
    add_button(layout, "NEW GAME ALPHA PLUS!", [=] { start_game(game_mode::kStandardRun); });
    add_button(layout, "Normal mode (legacy)", [=] { start_game(game_mode::kLegacy_Normal); });
    add_button(layout, "Boss mode (legacy)", [=] { start_game(game_mode::kLegacy_Boss); });
    add_button(layout, "Hard mode (legacy)", [=] { start_game(game_mode::kLegacy_Hard); });
    add_button(layout, "Fast mode (legacy)", [=] { start_game(game_mode::kLegacy_Fast); });
    add_button(layout, "W-hat mode (legacy)", [=] { start_game(game_mode::kLegacy_What); });
    add_button(layout, "Back", [this] { remove(); });

    stack.set_volume(1.f);
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
  set_bounds(irect{kUiDimensions});

  auto& layout = add_main_layout(this);
  if (stack.system().supports_networked_multiplayer()) {
    add_button(layout, "Start local game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ false); });
    add_button(layout, "Host online game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ true); });
    add_button(layout, "Join online game (not implemented, join via Steam UI instead)",
               [] { /* TODO */ });
  } else {
    add_button(layout, "Start game",
               [this] { this->stack().add<CreateGameLayer>(/* online */ false); });
  }
  add_button(layout, "View replay (not implemented)", [] { /* TODO */ });
  add_button(layout, "Exit", [this] { exit_timer_ = 8; });

  auto& top = add_main_layout(this, false);
  top_text_ = top.add_back<ui::TextElement>();
  top_text_->set_font({render::font_id::kMonospace, kMediumFont})
      .set_alignment(render::alignment::kTop | render::alignment::kLeft)
      .set_colour(kTextColour)
      .set_multiline(true);

  stack.set_volume(1.f);
  focus();
}

void MainMenuLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  ui::GameLayer::update_content(input, output);
  stack().set_fps(60);

  if (exit_timer_) {
    if (!--exit_timer_) {
      remove();
    }
    return;
  }

  if (auto e = event_triggered(stack().system(), System::event_type::kLobbyJoinRequested); e) {
    auto async = this->stack().system().join_lobby(e->id);
    this->stack().add<AsyncWaitLayer<void>>(
        ustring::ascii("Joining lobby..."), std::move(async),
        [this] { stack().add<RunLobbyLayer>(std::nullopt, /* online */ true); });
  }

  auto& system = stack().system();
  if (system.supports_networked_multiplayer()) {
    std::size_t friends_online = 0;
    std::size_t friends_in_game = 0;
    for (const auto& info : system.friend_list()) {
      friends_online += info.in_game ? 1u : 0u;
      friends_in_game += info.lobby_id ? 1u : 0u;
    }
    auto text = ustring::ascii("Version: " + std::string{kGameVersion} + "\nLogged in as: ") +
        system.local_user().name +
        ustring::ascii("\nFriends online: " + std::to_string(friends_online) +
                       "\nFriends in-game: " + std::to_string(friends_in_game));
    top_text_->set_text(std::move(text));
  }
}

}  // namespace ii
