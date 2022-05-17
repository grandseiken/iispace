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
  output_t* output_;
  Settings& settings_;
  std::int32_t selection_;
};

class HighScoreModal : public Modal {
public:
  HighScoreModal(SaveData& save, GameModal& game, const SimState::results& results);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  std::int64_t get_score() const;
  bool is_high_score() const;

  SaveData& save_;
  GameModal& game_;
  SimState::results results_;
  bool replay_;
  std::int32_t seed_;

  std::string enter_name_;
  std::int32_t enter_char_;
  std::int32_t enter_r_;
  std::int32_t enter_time_;
  std::int32_t compliment_;
  std::int32_t timer_;
};

class GameModal : public Modal {
public:
  GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count, game_mode mode,
            std::int32_t player_count, bool can_face_secret_boss);
  GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
            const std::string& replay_path);
  ~GameModal();

  const SimState& sim_state() const {
    return *state_;
  }
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  SaveData& save_;
  Settings& settings_;
  std::int32_t* frame_count_;
  PauseModal::output_t pause_output_ = PauseModal::kContinue;
  std::int32_t controllers_connected_ = 0;
  bool controllers_dialog_ = true;
  std::unique_ptr<SimState> state_;
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
    return lib_;
  }

private:
  game_mode mode_unlocked() const;

  enum class menu {
    kSpecial,
    kStart,
    kPlayers,
    kQuit,
  };

  Lib& lib_;
  std::int32_t frame_count_;

  menu menu_select_;
  std::int32_t player_select_;
  game_mode mode_select_;
  std::int32_t exit_timer_;
  std::string exit_error_;

  SaveData save_;
  Settings settings_;
  ModalStack modals_;
};

#endif
