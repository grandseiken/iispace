#ifndef IISPACE_SRC_STARS_H
#define IISPACE_SRC_STARS_H

#include "z0_game.h"

class Star {
public:

  static const int TIMER = 500;
  Star(z0Game& z0, float speed);
  virtual ~Star();

  static void Update();
  static void Render();
  static void CreateStar(z0Game& z0);
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
  z0Game& _z0;

  static Vec2f _direction;
  typedef std::vector<Star*> StarList;
  static StarList _starList;

};

class DotStar : public Star {
public:

  DotStar(z0Game& z0) : Star(z0, 18)
  {
    _c = z0.GetLib().RandInt(2) ? 0x222222ff : 0x333333ff;
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

  FarStar(z0Game& z0) : Star(z0, 10)
  {
    _c = z0.GetLib().RandInt(2) ? 0x222222ff : 0x111111ff;
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

  BigStar(z0Game& z0) : Star(z0, 14)
  {
    _c = z0.GetLib().RandInt(2) ? 0x111111ff : 0x222222ff;
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

  PlanetStar(z0Game& z0)
    : Star(z0, 10)
  {
    _n = z0.GetLib().RandInt(4) + 4;
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
