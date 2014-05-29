#include "stars.h"
#include "lib.h"

flvec2 Stars::_direction(0, 1);
int32_t Stars::_star_rate = 0;
std::vector<std::unique_ptr<Stars::data>> Stars::_stars;
static const uint32_t TIMER = 500;

void Stars::update()
{
  for (auto it = _stars.begin(); it != _stars.end();) {
    (*it)->position += _direction * (*it)->speed;
    (*it)->timer--;
    if ((*it)->timer <= 0) {
      it = _stars.erase(it);
      continue;
    }
    ++it;
  }

  int32_t r = _star_rate > 1 ? z::rand_int(_star_rate) : 0;
  for (int32_t i = 0; i < r; i++) {
    create_star();
  }
}

void Stars::change() {
  _direction = _direction.rotated((z::rand_fixed().to_float() - 0.5f) * M_PIf);
  for (const auto& star : _stars) {
    star->timer = TIMER;
  }
  _star_rate = z::rand_int(3) + 2;
}

void Stars::render(Lib& lib)
{
  for (const auto& star : _stars) {
    switch (star->type) {
    case DOT_STAR:
    case FAR_STAR:
      lib.RenderRect(star->position - flvec2(1, 1),
                     star->position + flvec2(1, 1), star->colour);
      break;

    case BIG_STAR:
      lib.RenderRect(star->position - flvec2(2, 2),
                     star->position + flvec2(2, 2), star->colour);
      break;

    case PLANET:
      for (int32_t i = 0; i < 8; i++) {
        flvec2 a = flvec2::from_polar(i * M_PIf / 4, star->size);
        flvec2 b = flvec2::from_polar((i + 1) * M_PIf / 4, star->size);
        lib.RenderLine(star->position + a, star->position + b, star->colour);
      }
    }
  }
}
void Stars::create_star()
{
  int32_t r = z::rand_int(12);
  if (r <= 0 && z::rand_int(4) != 0) {
    return;
  }

  type_t type = r <= 0 ? PLANET :
                r <= 3 ? BIG_STAR :
                r <= 7 ? FAR_STAR : DOT_STAR;
  float speed = type == DOT_STAR ? 18.f :
                type == BIG_STAR ? 14.f : 10.f;

  data* star = new data{TIMER, type, {}, speed, 0, 0};
  _stars.emplace_back(star);

  int32_t edge = z::rand_int(4);
  float ratio = z::rand_fixed().to_float();

  star->position.x = edge < 2 ? ratio * Lib::WIDTH :
      edge == 2 ? -16 : 16 + Lib::WIDTH;
  star->position.y = edge >= 2 ? ratio * Lib::HEIGHT :
      edge == 0 ? -16 : 16 + Lib::HEIGHT;

  star->colour =
      type == DOT_STAR ? (z::rand_int(2) ? 0x222222ff : 0x333333ff) :
      type == FAR_STAR ? (z::rand_int(2) ? 0x222222ff : 0x111111ff) :
      type == BIG_STAR ? (z::rand_int(2) ? 0x111111ff : 0x222222ff) :
      0x111111ff;
  if (type == PLANET) {
    star->size = 4.f + z::rand_int(4);
  }
}

void Stars::clear()
{
  _stars.clear();
  _direction = flvec2(1, 0);
  _star_rate = 0;
}