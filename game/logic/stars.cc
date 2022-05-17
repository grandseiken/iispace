#include "game/logic/stars.h"
#include "game/core/lib.h"

namespace {
const std::uint32_t kTimer = 500;
}  // namespace

fvec2 Stars::_direction(0, 1);
std::int32_t Stars::_star_rate = 0;
std::vector<std::unique_ptr<Stars::data>> Stars::_stars;

void Stars::update() {
  for (auto it = _stars.begin(); it != _stars.end();) {
    (*it)->position += _direction * (*it)->speed;
    (*it)->timer--;
    if ((*it)->timer <= 0) {
      it = _stars.erase(it);
      continue;
    }
    ++it;
  }

  std::int32_t r = _star_rate > 1 ? z::rand_int(_star_rate) : 0;
  for (std::int32_t i = 0; i < r; i++) {
    create_star();
  }
}

void Stars::change() {
  _direction = _direction.rotated((z::rand_fixed().to_float() - 0.5f) * M_PIf);
  for (const auto& star : _stars) {
    star->timer = kTimer;
  }
  _star_rate = z::rand_int(3) + 2;
}

void Stars::render(Lib& lib) {
  for (const auto& star : _stars) {
    switch (star->type) {
    case type::kDotStar:
    case type::kFarStar:
      lib.render_rect(star->position - fvec2(1, 1), star->position + fvec2(1, 1), star->colour);
      break;

    case type::kBigStar:
      lib.render_rect(star->position - fvec2(2, 2), star->position + fvec2(2, 2), star->colour);
      break;

    case type::kPlanet:
      for (std::int32_t i = 0; i < 8; i++) {
        fvec2 a = fvec2::from_polar(i * M_PIf / 4, star->size);
        fvec2 b = fvec2::from_polar((i + 1) * M_PIf / 4, star->size);
        lib.render_line(star->position + a, star->position + b, star->colour);
      }
    }
  }
}
void Stars::create_star() {
  std::int32_t r = z::rand_int(12);
  if (r <= 0 && z::rand_int(4) != 0) {
    return;
  }

  type t = r <= 0 ? type::kPlanet
      : r <= 3    ? type::kBigStar
      : r <= 7    ? type::kFarStar
                  : type::kDotStar;
  float speed = t == type::kDotStar ? 18.f : t == type::kBigStar ? 14.f : 10.f;

  data* star = new data{kTimer, t, {}, speed, 0, 0};
  _stars.emplace_back(star);

  std::int32_t edge = z::rand_int(4);
  float ratio = z::rand_fixed().to_float();

  star->position.x = edge < 2 ? ratio * Lib::kWidth : edge == 2 ? -16 : 16 + Lib::kWidth;
  star->position.y = edge >= 2 ? ratio * Lib::kHeight : edge == 0 ? -16 : 16 + Lib::kHeight;

  star->colour = t == type::kDotStar ? (z::rand_int(2) ? 0x222222ff : 0x333333ff)
      : t == type::kFarStar          ? (z::rand_int(2) ? 0x222222ff : 0x111111ff)
      : t == type::kBigStar          ? (z::rand_int(2) ? 0x111111ff : 0x222222ff)
                                     : 0x111111ff;
  if (t == type::kPlanet) {
    star->size = 4.f + z::rand_int(4);
  }
}

void Stars::clear() {
  _stars.clear();
  _direction = fvec2(1, 0);
  _star_rate = 0;
}