#ifndef IISPACE_SRC_POWERUP_H
#define IISPACE_SRC_POWERUP_H

#include "ship.h"

class Powerup : public Ship {
public:

  enum type_t {
    EXTRA_LIFE,
    MAGIC_SHOTS,
    SHIELD,
    BOMB,
  };

  Powerup(const vec2& position, type_t type);
  void Update() override;
  void Damage(int damage, bool magic, Player* source) override;

private:

  type_t _type;
  int32_t _frame;
  vec2 _dir;
  bool _rotate;
  bool _first_frame;

};

#endif
