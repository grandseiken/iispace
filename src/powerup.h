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

  Powerup(const Vec2& position, type_t type);
  virtual ~Powerup() {}

  void Update() override;
  void Damage(int damage, bool magic, Player* source) override;

  bool IsPowerup() const override
  {
    return true;
  }

private:

  type_t _type;
  int _frame;
  Vec2 _dir;
  bool _rotate;
  bool _first_frame;

};

#endif
