#ifndef II_GAME_CORE_Z0_GAME_H
#define II_GAME_CORE_Z0_GAME_H
#include "game/core/io_input_adapter.h"
#include "game/core/modal.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/logic/sim/sim_state.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace ii::io {
class IoLayer;
}  // namespace ii::io
class GameModal;

namespace ii {
struct game_options_t {
  std::uint32_t ai_count = 0;
};
}  // namespace ii

class PauseModal : public Modal {
public:
  enum output_t {
    kContinue,
    kEndGame,
  };

  PauseModal(output_t* output);
  void update(ii::ui::UiLayer& ui) override;
  void render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const override;

private:
  output_t* output_;
  std::uint32_t selection_ = 0;
};

class HighScoreModal : public Modal {
public:
  HighScoreModal(bool is_replay, GameModal& game, const ii::sim_results& results,
                 ii::ReplayWriter* replay_writer);
  void update(ii::ui::UiLayer& ui) override;
  void render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const override;

private:
  std::uint64_t get_score() const;
  bool is_high_score(const ii::SaveGame&) const;

  bool is_replay_ = false;
  GameModal& game_;
  ii::sim_results results_;
  ii::ReplayWriter* replay_writer_ = nullptr;

  std::string enter_name_;
  std::uint32_t enter_char_ = 0;
  std::uint32_t enter_r_ = 0;
  std::uint32_t enter_time_ = 0;
  std::size_t compliment_ = 0;
  std::uint32_t timer_ = 0;
};

class GameModal : public Modal {
public:
  GameModal(ii::io::IoLayer& io_layer, const ii::initial_conditions& conditions,
            const ii::game_options_t& options);
  GameModal(ii::ReplayReader&& replay);
  ~GameModal();

  void update(ii::ui::UiLayer& ui) override;
  void render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const override;

private:
  struct replay_t {
    replay_t(ii::ReplayReader&& r) : reader{std::move(r)}, input{reader} {}
    ii::ReplayReader reader;
    ii::ReplayInputAdapter input;
  };
  struct game_t {
    game_t(ii::io::IoLayer& io, ii::ReplayWriter&& w) : writer{std::move(w)}, input{io, &writer} {}
    ii::ReplayWriter writer;
    ii::IoInputAdapter input;
  };

  PauseModal::output_t pause_output_ = PauseModal::kContinue;
  std::uint32_t frame_count_multiplier_ = 1;
  std::uint32_t audio_tick_ = 0;
  bool show_controllers_dialog_ = true;
  bool controllers_dialog_ = true;

  std::optional<replay_t> replay_;
  std::optional<game_t> game_;
  std::unique_ptr<ii::SimState> state_;
};

class z0Game : public Modal {
public:
  static constexpr glm::vec4 kPanelText = {0.f, 0.f, .925f, 1.f};
  static constexpr glm::vec4 kPanelTran = {0.f, 0.f, .925f, .6f};
  static constexpr glm::vec4 kPanelBack = {0.f, 0.f, 0.f, 1.f};

  z0Game(const ii::game_options_t& options);
  void update(ii::ui::UiLayer& ui) override;
  void render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const override;

private:
  ii::game_mode mode_unlocked(const ii::SaveGame&) const;

  enum class menu {
    kSpecial,
    kStart,
    kPlayers,
    kQuit,
  };

  ii::game_options_t options_;
  menu menu_select_ = menu::kStart;
  std::uint32_t player_select_ = 1;
  ii::game_mode mode_select_ = ii::game_mode::kBoss;
  std::uint32_t exit_timer_ = 0;
};

#endif
