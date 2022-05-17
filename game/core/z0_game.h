#ifndef IISPACE_GAME_CORE_Z0_GAME_H
#define IISPACE_GAME_CORE_Z0_GAME_H
#include "game/common/z.h"
#include "game/core/lib.h"
#include "game/core/modal.h"
#include "game/core/replay.h"
#include "game/core/save.h"
#include "game/logic/sim_state.h"
#include <cstdint>
#include <memory>

struct Particle;
struct PlayerInput;
class GameModal;
class Overmind;
class Player;
class Ship;

class PauseModal : public Modal {
public:
  enum output_t {
    kContinue,
    kEndGame,
  };

  PauseModal(output_t* output, Settings& settings);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  output_t* _output;
  Settings& _settings;
  std::int32_t _selection;
};

class HighScoreModal : public Modal {
public:
  HighScoreModal(SaveData& save, GameModal& game, const SimState::results& results);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  std::int64_t get_score() const;
  bool is_high_score() const;

  SaveData& _save;
  GameModal& _game;
  SimState::results _results;
  bool _replay;
  std::int32_t _seed;

  std::string _enter_name;
  std::int32_t _enter_char;
  std::int32_t _enter_r;
  std::int32_t _enter_time;
  std::int32_t _compliment;
  std::int32_t _timer;
};

class GameModal : public Modal {
public:
  GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count, game_mode mode,
            std::int32_t player_count, bool can_face_secret_boss);
  GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
            const std::string& replay_path);
  ~GameModal();

  const SimState& sim_state() const {
    return *_state;
  }
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  SaveData& _save;
  Settings& _settings;
  std::int32_t* _frame_count;
  PauseModal::output_t _pause_output = PauseModal::kContinue;
  std::int32_t _controllers_connected = 0;
  bool _controllers_dialog = true;
  std::unique_ptr<SimState> _state;
};

struct score_finished {};
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
    return _lib;
  }

private:
  game_mode mode_unlocked() const;

  enum class menu {
    kSpecial,
    kStart,
    kPlayers,
    kQuit,
  };

  Lib& _lib;
  std::int32_t _frame_count;

  menu _menu_select;
  std::int32_t _player_select;
  game_mode _mode_select;
  std::int32_t _exit_timer;
  std::string _exit_error;

  SaveData _save;
  Settings _settings;
  ModalStack _modals;
};

#endif
