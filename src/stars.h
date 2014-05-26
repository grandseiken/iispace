#ifndef IISPACE_SRC_STARS_H
#define IISPACE_SRC_STARS_H

#include "z0_game.h"

class Star {
public:

  static const std::size_t TIMER = 500;
  Star(float speed);
  virtual ~Star();

  static void Update();
  static void Render(Lib& lib);
  static void CreateStar();
  static void SetDirection(const Vec2f& direction);
  static void Clear();

protected:

  Colour _c;

private:

  bool Move();
  void ResetTimer();
  virtual void RenderStar(Lib& lib, const Vec2f& position) = 0;

  int _timer;
  Vec2f _position;
  float _speed;

  static Vec2f _direction;
  typedef std::vector<Star*> StarList;
  static StarList _starList;

};

class DotStar : public Star {
public:

  DotStar()
    : Star(18)
  {
    _c = z::rand_int(2) ? 0x222222ff : 0x333333ff;
  }
  virtual ~DotStar() {}

private:

  virtual void RenderStar(Lib& lib, const Vec2f& position)
  {
    lib.RenderRect(position - Vec2f(1, 1), position + Vec2f(1, 1), _c);
  }

};

class FarStar : public Star {
public:

  FarStar()
    : Star(10)
  {
    _c = z::rand_int(2) ? 0x222222ff : 0x111111ff;
  }
  virtual ~FarStar() {}

private:

  virtual void RenderStar(Lib& lib, const Vec2f& position)
  {
    lib.RenderRect(position - Vec2f(1, 1), position + Vec2f(1, 1), _c);
  }

};

class BigStar : public Star {
public:

  BigStar()
    : Star(14)
  {
    _c = z::rand_int(2) ? 0x111111ff : 0x222222ff;
  }
  virtual ~BigStar() {}

private:

  virtual void RenderStar(Lib& lib, const Vec2f& position)
  {
    lib.RenderRect(position - Vec2f(2, 2), position + Vec2f(2, 2), _c);
  }

};

class PlanetStar : public Star {
public:

  PlanetStar()
    : Star(10)
  {
    _n = z::rand_int(4) + 4;
    _c = 0x111111ff;
  }
  virtual ~PlanetStar() {}

private:

  virtual void RenderStar(Lib& lib, const Vec2f& position)
  {
    for (int i = 0; i < 8; i++) {
      Vec2f a, b;
      a.SetPolar(i * M_PIf / 4, float(_n));
      b.SetPolar((i + 1) * M_PIf / 4, float(_n));
      lib.RenderLine(position + a, position + b, _c);
    }
  }

  int _n;

};

#endif
