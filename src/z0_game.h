#ifndef IISPACE_SRC_Z0_GAME_H
#define IISPACE_SRC_Z0_GAME_H

#include <memory>

#include "lib.h"
#include "modal.h"
#include "replay.h"
#include "save.h"
#include "z.h"

struct Particle;
struct PlayerInput;
class GameModal;
class Overmind;
class Player;
class Ship;

class PauseModal : public Modal {
public:
  enum output_t {
    CONTINUE,
    END_GAME,
  };

  PauseModal(output_t* output, Settings& settings);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  output_t* _output;
  Settings& _settings;
  int32_t _selection;
};

class HighScoreModal : public Modal {
public:
  HighScoreModal(SaveData& save, GameModal& game, Overmind& overmind, bool replay, int32_t seed);
  void update(Lib& lib) override;
  void render(Lib& lib) const override;

private:
  int64_t get_score() const;
  bool is_high_score() const;

  SaveData& _save;
  GameModal& _game;
  Overmind& _overmind;
  bool _replay;
  int32_t _seed;

  std::string _enter_name;
  int32_t _enter_char;
  int32_t _enter_r;
  int32_t _enter_time;
  int32_t _compliment;
  int32_t _timer;
};

class GameModal : public Modal {
public:
  enum boss_list {
    BOSS_1A = 1,
    BOSS_1B = 2,
    BOSS_1C = 4,
    BOSS_2A = 8,
    BOSS_2B = 16,
    BOSS_2C = 32,
    BOSS_3A = 64,
  };

  GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count, Mode::mode mode,
            int32_t player_count, bool can_face_secret_boss);
  GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count,
            const std::string& replay_path);
  ~GameModal();

  void update(Lib& lib) override;
  void render(Lib& lib) const override;

  Lib& lib();
  Mode::mode mode() const;
  void write_replay(const std::string& team_name, int64_t score) const;

  typedef std::vector<Ship*> ship_list;
  void add_ship(Ship* ship);
  void add_particle(const Particle& particle);
  int32_t get_non_wall_count() const;

  ship_list all_ships(int32_t ship_mask = 0) const;
  ship_list ships_in_radius(const vec2& point, fixed radius, int32_t ship_mask = 0) const;
  ship_list collision_list(const vec2& point, int32_t category) const;
  bool any_collision(const vec2& point, int32_t category) const;

  int32_t alive_players() const;
  int32_t killed_players() const;
  Player* nearest_player(const vec2& point) const;
  const ship_list& players() const;

  void add_life();
  void sub_life();
  int32_t get_lives() const;

  void render_hp_bar(float fill);
  void set_boss_killed(boss_list boss);

private:
  GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count, Replay&& replay,
            bool replay_recording);

  Lib& _lib;
  SaveData& _save;
  Settings& _settings;
  PauseModal::output_t _pause_output;
  int32_t* _frame_count;
  int32_t _kill_timer;

  Replay _replay;
  bool _replay_recording;
  std::unique_ptr<PlayerInput> _input;

  int32_t _lives;
  std::unique_ptr<Overmind> _overmind;
  std::vector<Particle> _particles;
  std::vector<std::unique_ptr<Ship>> _ships;
  ship_list _player_list;
  ship_list _collisions;

  int32_t _controllers_connected;
  bool _controllers_dialog;

  mutable bool _show_hp_bar;
  mutable float _fill_hp_bar;
};

struct score_finished {};
class z0Game {
public:
  static const colour_t PANEL_TEXT = 0xeeeeeeff;
  static const colour_t PANEL_TRAN = 0xeeeeee99;
  static const colour_t PANEL_BACK = 0x000000ff;

  z0Game(Lib& lib, const std::vector<std::string>& args);

  void run();
  bool update();
  void render() const;

  Lib& lib() const {
    return _lib;
  }

private:
  Mode::mode mode_unlocked() const;

  enum menu_t {
    MENU_SPECIAL,
    MENU_START,
    MENU_PLAYERS,
    MENU_QUIT,
  };

  Lib& _lib;
  int32_t _frame_count;

  menu_t _menu_select;
  int32_t _player_select;
  Mode::mode _mode_select;
  int32_t _exit_timer;
  std::string _exit_error;

  SaveData _save;
  Settings _settings;
  ModalStack _modals;
};

#endif
