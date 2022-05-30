#ifndef IISPACE_GAME_CORE_Z0_GAME_H
#define IISPACE_GAME_CORE_Z0_GAME_H
#include "game/core/lib.h"
#include "game/core/modal.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/logic/sim/sim_state.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

class GameModal;

class PauseModal : public Modal {
public:
  enum output_t {
    kContinue,
    kEndGame,
  };

  PauseModal(output_t* output, ii::Config& settings);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  output_t* output_;
  ii::Config& settings_;
  std::int32_t selection_ = 0;
};

class HighScoreModal : public Modal {
public:
  HighScoreModal(bool is_replay, ii::SaveGame& save, GameModal& game,
                 const ii::sim_results& results, ii::ReplayWriter* replay_writer);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  std::int64_t get_score() const;
  bool is_high_score() const;

  bool is_replay_ = false;
  ii::SaveGame& save_;
  GameModal& game_;
  ii::sim_results results_;
  ii::ReplayWriter* replay_writer_ = nullptr;

  std::string enter_name_;
  std::int32_t enter_char_ = 0;
  std::int32_t enter_r_ = 0;
  std::int32_t enter_time_ = 0;
  std::int32_t compliment_ = 0;
  std::int32_t timer_ = 0;
};

class GameModal : public Modal {
public:
  using frame_count_callback = std::function<void(std::int32_t)>;
  GameModal(Lib& lib, ii::SaveGame& save, ii::Config& settings,
            const frame_count_callback& callback, const ii::initial_conditions& conditions);
  GameModal(Lib& lib, ii::SaveGame& save, ii::Config& settings,
            const frame_count_callback& callback, const std::string& replay_path);
  ~GameModal();

  const ii::SimState& sim_state() const {
    return *state_;
  }
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  struct replay_t {
    replay_t(ii::ReplayReader&& r) : reader{std::move(r)}, input{reader} {}
    ii::ReplayReader reader;
    ii::ReplayInputAdapter input;
  };
  struct game_t {
    game_t(Lib& lib, ii::ReplayWriter&& w) : writer{std::move(w)}, input{lib, writer} {}
    ii::ReplayWriter writer;
    LibInputAdapter input;
  };

  ii::SaveGame& save_;
  ii::Config& settings_;
  frame_count_callback callback_;
  PauseModal::output_t pause_output_ = PauseModal::kContinue;
  std::int32_t controllers_connected_ = 0;
  std::int32_t frame_count_multiplier_ = 1;
  mutable std::int32_t audio_tick_ = 0;
  bool controllers_dialog_ = true;

  std::optional<replay_t> replay_;
  std::optional<game_t> game_;
  std::unique_ptr<ii::SimState> state_;
};

class z0Game {
public:
  static constexpr colour_t kPanelText = 0xeeeeeeff;
  static constexpr colour_t kPanelTran = 0xeeeeee99;
  static constexpr colour_t kPanelBack = 0x000000ff;

  z0Game(Lib& lib, const std::vector<std::string>& args);

  void run();
  bool update();
  void render() const;

  Lib& lib() const {
    return lib_;
  }

private:
  ii::game_mode mode_unlocked() const;

  enum class menu {
    kSpecial,
    kStart,
    kPlayers,
    kQuit,
  };

  Lib& lib_;

  menu menu_select_ = menu::kStart;
  std::int32_t player_select_ = 1;
  ii::game_mode mode_select_ = ii::game_mode::kBoss;
  std::int32_t exit_timer_ = 0;
  std::string exit_error_;
  std::int32_t frame_count_ = 1;

  ii::SaveGame save_;
  ii::Config settings_;
  ModalStack modals_;
};

#endif
