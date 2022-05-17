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

  enum class type {
    kDotStar,
    kFarStar,
    kBigStar,
    kPlanet,
  };

  struct data {
    std::int32_t timer;
    Stars::type type;
    fvec2 position;
    float speed;
    float size;
    colour_t colour;
  };

  static fvec2 direction_;
  static std::vector<std::unique_ptr<data>> stars_;
  static std::int32_t star_rate_;
};

#endif
