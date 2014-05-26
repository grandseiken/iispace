#include "stars.h"

Vec2f Star::_direction(0, 1);
Star::StarList Star::_starList;

Star::Star(float speed)
  : _timer(TIMER)
  , _speed(speed)
{
  _starList.push_back(this);
  int edge = z::rand_int(4);
  float ratio = z::rand_fixed().to_float();

  if (edge < 2) {
    _position._x = ratio * Lib::WIDTH;
  }
  else {
    _position._y = ratio * Lib::HEIGHT;
  }

  if (edge == 0) {
    _position._y = -16;
  }
  if (edge == 1) {
    _position._y = Lib::HEIGHT + 16;
  }
  if (edge == 2) {
    _position._x = -16;
  }
  if (edge == 3) {
    _position._x = Lib::WIDTH + 16;
  }
}

Star::~Star()
{
}

void Star::Update()
{
  for (std::size_t i = 0; i < _starList.size(); i++) {
    if (!_starList[i]->Move()) {
      delete _starList[i];
      _starList.erase(_starList.begin() + i);
      i--;
    }
  }
}

void Star::Render(Lib& lib)
{
  for (std::size_t i = 0; i < _starList.size(); i++) {
    _starList[i]->RenderStar(lib, _starList[i]->_position);
  }
}

void Star::CreateStar()
{
  Star* s = 0;
  int r = z::rand_int(12);

  if (r <= 0) {
    if (z::rand_int(4) == 0) {
      s = new PlanetStar();
    }
  }
  else if (r <= 3) {
    s = new BigStar();
  }
  else if (r <= 7) {
    s = new FarStar();
  }
  else {
    s = new DotStar();
  }
}

void Star::SetDirection(const Vec2f& direction)
{
  _direction = direction;
  for (std::size_t i = 0; i < _starList.size(); i++) {
    _starList[i]->ResetTimer();
  }
}

void Star::Clear()
{
  for (std::size_t i = 0; i < _starList.size(); i++) {
    delete _starList[i];
  }
  _starList.clear();
}

bool Star::Move()
{
  _position += _direction * _speed;
  _timer--;
  return _timer > 0;
}

void Star::ResetTimer()
{
  _timer = TIMER;
}
