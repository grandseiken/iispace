#include "game/logic/stars.h"
#include "game/logic/shape.h"
#include "game/logic/sim/sim_interface.h"
#include <glm/gtc/constants.hpp>

namespace {
const std::uint32_t kTimer = 500;
}  // namespace

glm::vec2 Stars::direction_ = {0, 1};
std::int32_t Stars::star_rate_ = 0;
std::vector<std::unique_ptr<Stars::data>> Stars::stars_;

void Stars::update(ii::SimInterface& sim) {
  for (auto it = stars_.begin(); it != stars_.end();) {
    (*it)->position += direction_ * (*it)->speed;
    (*it)->timer--;
    if ((*it)->timer <= 0) {
      it = stars_.erase(it);
      continue;
    }
    ++it;
  }

  std::int32_t r = star_rate_ > 1 ? sim.random(star_rate_) : 0;
  for (std::int32_t i = 0; i < r; ++i) {
    create_star(sim);
  }
}

void Stars::change(ii::SimInterface& sim) {
  direction_ = rotate(direction_, (sim.random_fixed().to_float() - 0.5f) * glm::pi<float>());
  for (const auto& star : stars_) {
    star->timer = kTimer;
  }
  star_rate_ = sim.random(3) + 2;
}

void Stars::render(const ii::SimInterface& sim) {
  for (const auto& star : stars_) {
    switch (star->type) {
    case type::kDotStar:
    case type::kFarStar:
      sim.render_line_rect(star->position - glm::vec2{1, 1}, star->position + glm::vec2{1, 1},
                           star->colour);
      break;

    case type::kBigStar:
      sim.render_line_rect(star->position - glm::vec2{2, 2}, star->position + glm::vec2{2, 2},
                           star->colour);
      break;

    case type::kPlanet:
      for (std::int32_t i = 0; i < 8; ++i) {
        auto a = from_polar(i * glm::pi<float>() / 4, star->size);
        auto b = from_polar((i + 1) * glm::pi<float>() / 4, star->size);
        sim.render_line(star->position + a, star->position + b, star->colour);
      }
    }
  }
}
void Stars::create_star(ii::SimInterface& sim) {
  std::int32_t r = sim.random(12);
  if (r <= 0 && sim.random(4) != 0) {
    return;
  }

  type t = r <= 0 ? type::kPlanet
      : r <= 3    ? type::kBigStar
      : r <= 7    ? type::kFarStar
                  : type::kDotStar;
  float speed = t == type::kDotStar ? 18.f : t == type::kBigStar ? 14.f : 10.f;

  auto star = std::make_unique<data>();
  star->timer = kTimer;
  star->type = t;
  star->speed = speed;

  std::int32_t edge = sim.random(4);
  float ratio = sim.random_fixed().to_float();

  star->position.x = edge < 2 ? ratio * ii::kSimWidth : edge == 2 ? -16 : 16 + ii::kSimWidth;
  star->position.y = edge >= 2 ? ratio * ii::kSimHeight : edge == 0 ? -16 : 16 + ii::kSimHeight;

  star->colour = t == type::kDotStar ? (sim.random(2) ? 0x222222ff : 0x333333ff)
      : t == type::kFarStar          ? (sim.random(2) ? 0x222222ff : 0x111111ff)
      : t == type::kBigStar          ? (sim.random(2) ? 0x111111ff : 0x222222ff)
                                     : 0x111111ff;
  if (t == type::kPlanet) {
    star->size = 4.f + sim.random(4);
  }
  stars_.emplace_back(std::move(star));
}

void Stars::clear() {
  stars_.clear();
  direction_ = {1, 0};
  star_rate_ = 0;
}