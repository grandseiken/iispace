#ifndef IISPACE_GAME_LOGIC_STARS_H
#define IISPACE_GAME_LOGIC_STARS_H
#include "game/common/z.h"
#include <memory>
#include <vector>

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
    fvec2 position;
    float speed;
    float size;
    colour_t colour;
  };

  static fvec2 _direction;
  static std::vector<std::unique_ptr<data>> _stars;
  static int32_t _star_rate;
};

#endif
