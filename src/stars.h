#ifndef IISPACE_SRC_STARS_H
#define IISPACE_SRC_STARS_H

#include <memory>
#include <vector>
#include "z.h"
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
    flvec2 position;
    float speed;
    float size;
    colour_t colour;
  };

  static flvec2 _direction;
  static std::vector<std::unique_ptr<data>> _stars;
  static int32_t _star_rate;
};

#endif
