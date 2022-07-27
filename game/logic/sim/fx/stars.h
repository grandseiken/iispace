#ifndef II_GAME_LOGIC_SIM_FX_STARS_H
#define II_GAME_LOGIC_SIM_FX_STARS_H
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace ii {
class SimInterface;

class Stars {
public:
  void change(SimInterface& sim);
  void update(SimInterface& sim);
  void render(const SimInterface& sim);

private:
  void create_star(SimInterface& sim);

  enum class type {
    kDotStar,
    kFarStar,
    kBigStar,
    kPlanet,
  };

  struct data_t {
    std::uint32_t timer = 0;
    Stars::type type = Stars::type::kDotStar;
    glm::vec2 position{0.f};
    float speed = 0;
    float size = 0;
    glm::vec4 colour{0.f};
  };

  glm::vec2 direction_ = {0, 1};
  std::vector<data_t> stars_;
  std::uint32_t star_rate_ = 0;
};

}  // namespace ii

#endif
