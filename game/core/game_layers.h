#ifndef II_GAME_CORE_GAME_LAYERS_H
#define II_GAME_CORE_GAME_LAYERS_H
#include "game/core/io_input_adapter.h"
#include "game/core/render_state.h"
#include "game/core/ui/game_stack.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/logic/sim/sim_state.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <random>

namespace ii::io {
class IoLayer;
}  // namespace ii::io

namespace ii {
struct game_options_t {
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::vector<std::uint32_t> ai_players;
  std::vector<std::uint32_t> replay_remote_players;
  std::uint64_t replay_min_tick_delivery_delay = 0;
  std::uint64_t replay_max_tick_delivery_delay = 0;
};

class PauseLayer : public ui::GameLayer {
public:
  enum output_t {
    kContinue,
    kEndGame,
  };

  PauseLayer(ui::GameStack& stack, output_t* output);
  ~PauseLayer() override = default;
  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer& r) const override;

private:
  output_t* output_ = nullptr;
  std::uint32_t selection_ = 0;
};

class SimLayer : public ui::GameLayer {
public:
  SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
           const game_options_t& options);
  SimLayer(ui::GameStack& stack, data::ReplayReader&& replay, const game_options_t& options);
  ~SimLayer() override;

  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer& r) const override;

private:
  struct replay_t {
    replay_t(data::ReplayReader&& r) : reader{std::move(r)} {}
    data::ReplayReader reader;
  };
  struct game_t {
    game_t(io::IoLayer& io, data::ReplayWriter&& w) : writer{std::move(w)}, input{io} {}
    data::ReplayWriter writer;
    IoInputAdapter input;
  };

  PauseLayer::output_t pause_output_ = PauseLayer::kContinue;
  std::uint32_t frame_count_multiplier_ = 1;
  std::uint32_t audio_tick_ = 0;
  bool show_controllers_dialog_ = true;
  bool controllers_dialog_ = true;

  struct replay_network_packet {
    std::uint64_t delivery_tick_count = 0;
    sim_packet packet;
  };

  std::mt19937_64 engine_;
  game_options_t options_;
  game_mode mode_;
  RenderState render_state_;
  std::optional<replay_t> replay_;
  std::optional<game_t> game_;
  std::vector<replay_network_packet> replay_packets_;
  std::unique_ptr<SimState> state_;
  std::unique_ptr<NetworkedSimState> network_state_;
};

class MainMenuLayer : public ui::GameLayer {
public:
  MainMenuLayer(ui::GameStack& stack, const game_options_t& options);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  enum class menu {
    kSpecial,
    kStart,
    kPlayers,
    kQuit,
  };

  game_options_t options_;
  menu menu_select_ = menu::kStart;
  std::uint32_t player_select_ = 1;
  game_mode mode_select_ = game_mode::kBoss;
  std::uint32_t exit_timer_ = 0;
};

}  // namespace ii

#endif
