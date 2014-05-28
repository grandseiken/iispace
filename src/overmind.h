#ifndef IISPACE_SRC_OVERMIND_H
#define IISPACE_SRC_OVERMIND_H

#include "z.h"
class Ship;
class z0Game;

class Overmind {
public:

  static const int TIMER = 2800;
  static const int POWERUP_TIME = 1200;
  static const int BOSS_REST_TIME = 240;

  static const int INITIAL_POWER = 16;
  static const int INITIAL_TRIGGERVAL = 0;
  static const int LEVELS_PER_GROUP = 4;
  static const int BASE_GROUPS_PER_BOSS = 4;

  Overmind(z0Game& z0);
  ~Overmind() {}

  // General
  //------------------------------
  int GetPower() const
  {
    return _power;
  }

  void Reset(bool canFaceSecretBoss);

  void Update();

  int  GetKilledBosses() const
  {
    return _bossModBosses - 1;
  }

  long GetElapsedTime() const
  {
    return _elapsedTime;
  }

  int GetTimer() const
  {
    if (_isBossLevel) {
      return -1;
    }
    return TIMER - _timer;
  }

  // Enemy-counting
  //------------------------------
  void OnEnemyDestroy(const Ship& ship);
  void OnEnemyCreate(const Ship& ship);

  int CountEnemies() const
  {
    return _count;
  }

  int CountNonWallEnemies() const
  {
    return _nonWallCount;
  }

private:

  // Individual functions
  //------------------------------
  void SpawnPowerup();
  void SpawnBossReward();

  static void SpawnFollow(int num, int div, int side);
  static void SpawnChaser(int num, int div, int side);
  static void SpawnSquare(int num, int div, int side);
  static void SpawnWall(int num, int div, int side, bool dir);
  static void SpawnFollowHub(int num, int div, int side);
  static void SpawnShielder(int num, int div, int side);
  static void SpawnTractor(int num, int div, int side);

  // Internals
  //------------------------------
  static void spawn(Ship* ship);
  static vec2 SpawnPoint(bool top, int row, int num, int div);

  void Wave();
  void Boss();
  void BossModeBoss();

  z0Game& _z0;
  static int _power;
  int _timer;
  int _count;
  int _nonWallCount;
  int _countTrigger;
  int _levelsMod;
  int _groupsMod;
  int _bossModBosses;
  int _bossModFights;
  int _bossModSecret;
  bool _canFaceSecretBoss;
  int _powerupMod;
  int _livesTarget;
  bool _isBossNext;
  bool _isBossLevel;
  long _startTime;
  long _elapsedTime;
  bool _timeStopped;
  int _bossRestTimer;
  int _wavesTotal;
  static int _hardAlready;

  std::vector<int> _boss1Queue;
  std::vector<int> _boss2Queue;
  int _bossesToGo;

  typedef std::pair<int, int> FormationCost;
  typedef FormationCost(*SpawnFormationFunction)(bool query, int row);
  typedef std::pair<FormationCost, SpawnFormationFunction> Formation;
  typedef std::vector<Formation> FormationList;

  FormationList _formations;
  void AddFormation(SpawnFormationFunction function);

  static bool SortFormations(const Formation& a, const Formation& b)
  {
    return a.first.first < b.first.first;
  }

  // Formation hacks
  //------------------------------
  static int _tRow;
  static z0Game* _tz0;
  void AddFormations();

#define FORM_DEC(name) static FormationCost name (bool query, int row)
#define FORM_USE(name) AddFormation(&name)
#define FORM_DEF(name, cost, minResource)\
    Overmind::FormationCost Overmind:: name(bool query, int row) {\
      if (query) {\
        return FormationCost(cost, minResource);\
      }\
      _tRow = row;
#define FORM_END return FormationCost(0, 0); }

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
