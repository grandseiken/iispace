#ifndef IISPACE_SRC_OVERMIND_H
#define IISPACE_SRC_OVERMIND_H

#include "z.h"
#include <vector>
class Ship;
class z0Game;

class Overmind {
public:

  static const int32_t TIMER = 2800;
  static const int32_t POWERUP_TIME = 1200;
  static const int32_t BOSS_REST_TIME = 240;

  static const int32_t INITIAL_POWER = 16;
  static const int32_t INITIAL_TRIGGERVAL = 0;
  static const int32_t LEVELS_PER_GROUP = 4;
  static const int32_t BASE_GROUPS_PER_BOSS = 4;

  Overmind(z0Game& z0);
  ~Overmind() {}

  // General
  //------------------------------
  void reset(bool can_face_secret_boss);
  void update();

  int32_t get_killed_bosses() const
  {
    return _boss_mod_bosses - 1;
  }

  int32_t get_elapsed_time() const
  {
    return _elapsed_time;
  }

  int32_t get_timer() const
  {
    return _is_boss_level ? -1 : TIMER - _timer;
  }

  // Enemy-counting
  //------------------------------
  void on_enemy_destroy(const Ship& ship);
  void on_enemy_create(const Ship& ship);

  int32_t count_non_wall_enemies() const
  {
    return _non_wall_count;
  }

private:

  void spawn_powerup();
  void spawn_boss_reward();
  static void spawn_follow(int32_t num, int32_t div, int32_t side);
  static void spawn_chaser(int32_t num, int32_t div, int32_t side);
  static void spawn_square(int32_t num, int32_t div, int32_t side);
  static void spawn_wall(int32_t num, int32_t div, int32_t side, bool dir);
  static void spawn_follow_hub(int32_t num, int32_t div, int32_t side);
  static void spawn_shielder(int32_t num, int32_t div, int32_t side);
  static void spawn_tractor(int32_t num, int32_t div, int32_t side);

  static void spawn(Ship* ship);
  static vec2 spawn_point(bool top, int32_t row, int32_t num, int32_t div);

  void wave();
  void boss();
  void boss_mode_boss();

  z0Game& _z0;
  static int32_t _power;
  int32_t _timer;
  int32_t _count;
  int32_t _non_wall_count;
  int32_t _levels_mod;
  int32_t _groups_mod;
  int32_t _boss_mod_bosses;
  int32_t _boss_mod_fights;
  int32_t _boss_mod_secret;
  bool _can_face_secret_boss;
  int32_t _powerup_mod;
  int32_t _lives_target;
  bool _is_boss_next;
  bool _is_boss_level;
  int32_t _elapsed_time;
  int32_t _boss_rest_timer;
  int32_t _waves_total;
  static int32_t _hard_already;

  std::vector<int32_t> _boss1_queue;
  std::vector<int32_t> _boss2_queue;
  int32_t _bosses_to_go;

  typedef std::pair<int32_t, int32_t> formation_cost;
  typedef formation_cost(*spawn_formation_function)(bool query, int32_t row);
  typedef std::pair<formation_cost, spawn_formation_function> formation;
  typedef std::vector<formation> formation_list;

  formation_list _formations;
  void add_formation(spawn_formation_function function);

  static bool sort_formations(const formation& a, const formation& b)
  {
    return a.first.first < b.first.first;
  }

  // Formation hacks
  //------------------------------
  static int32_t _trow;
  static z0Game* _tz0;
  void add_formations();

#define FORM_DEC(name) static formation_cost name (bool query, int32_t row)
#define FORM_USE(name) add_formation(&name)
#define FORM_DEF(name, cost, minResource)\
    Overmind::formation_cost Overmind:: name(bool query, int32_t row) {\
      if (query) {\
        return formation_cost(cost, minResource);\
      }\
      _trow = row;
#define FORM_END return formation_cost(0, 0); }

  // Formations
  //------------------------------
  FORM_DEC(Square1);
  FORM_DEC(Square2);
  FORM_DEC(Square3);
  FORM_DEC(Square1Side);
  FORM_DEC(Square2Side);
  FORM_DEC(Square3Side);
  FORM_DEC(Wall1);
  FORM_DEC(Wall2);
  FORM_DEC(Wall3);
  FORM_DEC(Wall1Side);
  FORM_DEC(Wall2Side);
  FORM_DEC(Wall3Side);
  FORM_DEC(Follow1);
  FORM_DEC(Follow2);
  FORM_DEC(Follow3);
  FORM_DEC(Follow1Side);
  FORM_DEC(Follow2Side);
  FORM_DEC(Follow3Side);
  FORM_DEC(Chaser1);
  FORM_DEC(Chaser2);
  FORM_DEC(Chaser3);
  FORM_DEC(Chaser4);
  FORM_DEC(Chaser1Side);
  FORM_DEC(Chaser2Side);
  FORM_DEC(Chaser3Side);
  FORM_DEC(Chaser4Side);
  FORM_DEC(Hub1);
  FORM_DEC(Hub2);
  FORM_DEC(Hub1Side);
  FORM_DEC(Hub2Side);
  FORM_DEC(Mixed1);
  FORM_DEC(Mixed2);
  FORM_DEC(Mixed3);
  FORM_DEC(Mixed4);
  FORM_DEC(Mixed5);
  FORM_DEC(Mixed6);
  FORM_DEC(Mixed7);
  FORM_DEC(Mixed1Side);
  FORM_DEC(Mixed2Side);
  FORM_DEC(Mixed3Side);
  FORM_DEC(Mixed4Side);
  FORM_DEC(Mixed5Side);
  FORM_DEC(Mixed6Side);
  FORM_DEC(Mixed7Side);
  FORM_DEC(Tractor1);
  FORM_DEC(Tractor2);
  FORM_DEC(Tractor1Side);
  FORM_DEC(Tractor2Side);
  FORM_DEC(Shielder1);
  FORM_DEC(Shielder1Side);

};

#endif
