#ifndef II_GAME_LOGIC_STARS_H
#define II_GAME_LOGIC_STARS_H
#include "game/common/z.h"
#include <memory>
#include <vector>

namespace ii {
class SimInterface;
}  // namespace ii

class Stars {
public:
  static void change(ii::SimInterface& sim);
  static void update(ii::SimInterface& sim);
  static void render(const ii::SimInterface& sim);
  static void clear();

private:
  static void create_star(ii::SimInterface& sim);

  enum class type {
    kDotStar,
    kFarStar,
    kBigStar,
    kPlanet,
  };

  struct data {
    std::int32_t timer = 0;
    Stars::type type = Stars::type::kDotStar;
    glm::vec2 position{0.f};
    float speed = 0;
    float size = 0;
    colour_t colour = 0;
  };

  static glm::vec2 direction_;
  static std::vector<std::unique_ptr<data>> stars_;
  static std::int32_t star_rate_;
};

#endif
