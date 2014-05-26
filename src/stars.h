#ifndef IISPACE_SRC_STARS_H
#define IISPACE_SRC_STARS_H

#include "z.h"
#include <memory>
class Lib;

class Stars {
public:

  static void change();
  static void update();
  static void render(Lib& lib);
  static void clear();

private:

  static void create_star();

  enum type_t {
    DOT_STAR,
    FAR_STAR,
    BIG_STAR,
    PLANET,
  };

  struct data {
    int32_t timer;
    type_t type;
    Vec2f position;
    float speed;
    float size;
    colour colour;
  };

  static Vec2f _direction;
  static std::vector<std::unique_ptr<data>> _stars;
  static int32_t _star_rate;

};

#endif
