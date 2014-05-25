#include "stars.h"

Vec2f Star::_direction(0, 1);
Star::StarList Star::_starList;

Star::Star(z0Game& z0, float speed)
  : _timer(TIMER)
  , _speed(speed)
  , _z0(z0)
{
  _starList.push_back(this);
  int edge = _z0.GetLib().RandInt(4);
  float ratio = _z0.GetLib().RandFloat().to_float();

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

void Star::Render()
{
  for (std::size_t i = 0; i < _starList.size(); i++) {
    _starList[i]->RenderStar(
        _starList[i]->_z0.GetLib(), _starList[i]->_position);
  }
}

void Star::CreateStar(z0Game& z0)
{
  Star* s = 0;
  int r = z0.GetLib().RandInt(12);

  if (r <= 0) {
    if (z0.GetLib().RandInt(4) == 0) {
      s = new PlanetStar(z0);
    }
  }
  else if (r <= 3) {
    s = new BigStar(z0);
  }
  else if (r <= 7) {
    s = new FarStar(z0);
  }
  else {
    s = new DotStar(z0);
  }

  if (s != 0 && z0.GetLib().LoadSettings()._disableBackground) {
    s->_c = 0;
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
