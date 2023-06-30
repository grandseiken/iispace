#ifndef II_GAME_COMMON_COLOURS_H
#define II_GAME_COMMON_COLOURS_H
#include "game/common/math.h"
#include <gcem.hpp>
#include <glm/mat3x3.hpp>
#include <cmath>

namespace ii::colour {

inline constexpr cvec4 clamp(const cvec4& c) {
  auto f = [](float x) { return x < 0.f ? 0.f : x > 1.f ? 1.f : x; };
  return {f(c.x), f(c.y), f(c.z), f(c.w)};
}

inline constexpr cvec4 alpha(const cvec4& c, float a) {
  return clamp({c.x, c.y, c.z, c.w * a});
}

inline constexpr cvec4 srgb2rgb(const cvec4& c) {
  auto f = [](float v) constexpr {
    if (v < .04045f) {
      return v / 12.92f;
    }
    if (std::is_constant_evaluated()) {
      return gcem::pow((v + .055f) / 1.055f, 2.4f);
    }
    return std::pow((v + .055f) / 1.055f, 2.4f);
  };
  return {f(c.x), f(c.y), f(c.z), c.w};
}

inline constexpr cvec4 rgb2srgb(const cvec4& c) {
  auto f = [](float v) constexpr {
    if (v < .0031308f) {
      return v * 12.92f;
    }
    if (std::is_constant_evaluated()) {
      return 1.055f * gcem::pow(v, 1.f / 2.4f) - .055f;
    }
    return 1.055f * std::pow(v, 1.f / 2.4f) - .055f;
  };
  return {f(c.x), f(c.y), f(c.z), c.w};
}

inline constexpr cvec4 hsl2srgb(const cvec4& c) {
  float h = std::is_constant_evaluated() ? gcem::fmod(c.x, 1.f) : std::fmod(c.x, 1.f);
  float s = c.y;
  float l = c.z;
  float v = l <= .5f ? l * (1.f + s) : l + s - l * s;

  if (v == 0.f) {
    return {0.f, 0.f, 0.f, c.w};
  }
  float m = 2.f * l - v;
  float sv = (v - m) / v;
  float sextant = h * 6.f;
  float vsf =
      v * sv * (std::is_constant_evaluated() ? gcem::fmod(h * 6.f, 1.f) : std::fmod(h * 6.f, 1.f));
  float mid1 = m + vsf;
  float mid2 = v - vsf;

  if (sextant < 1.f) {
    return {v, mid1, m, c.w};
  }
  if (sextant < 2.f) {
    return {mid2, v, m, c.w};
  }
  if (sextant < 3.f) {
    return {m, v, mid1, c.w};
  }
  if (sextant < 4.f) {
    return {m, mid2, v, c.w};
  }
  if (sextant < 5.f) {
    return {mid1, m, v, c.w};
  }
  return {v, m, mid2, c.w};
}

inline constexpr cvec4 srgb2hsl(const cvec4& c) {
  auto v = std::max(c.x, std::max(c.y, c.z));
  auto m = std::min(c.x, std::min(c.y, c.z));
  auto l = (m + v) / 2.f;
  if (l <= 0.f) {
    return cvec4{0.f, 0.f, 0.f, c.w};
  }
  auto vm = v - m;
  auto s = vm;
  if (s <= 0.f) {
    return cvec4{0.f, 0.f, l, c.w};
  }
  s /= l <= .5f ? v + m : 2.f - v - m;

  auto r = (v - c.x) / vm;
  auto g = (v - c.y) / vm;
  auto b = (v - c.z) / vm;
  float h = 0.f;
  if (c.x == v) {
    h = (c.y == m ? 5.f + b : 1.f - g);
  } else if (c.y == v) {
    h = (c.z == m ? 1.f + r : 3.f - b);
  } else {
    h = (c.x == m ? 3.f + g : 5.f - r);
  }
  h /= 6.f;
  return {h, s, l, c.w};
}

inline constexpr cvec4 srgb2hsl(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return srgb2hsl({static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f,
                   static_cast<float>(b) / 255.f, 1.f});
}

constexpr glm::mat3 kRgb2Lms{.4122214708f, .5363325363f, .0514459929f, .2119034982f, .6806995451f,
                             .1073969566f, .0883024619f, .2817188376f, .6299787005f};
constexpr glm::mat3 kLms2Lab{.2104542553f,  .7936177850f,   -.0040720468f,
                             1.9779984951f, -2.4285922050f, .4505937099f,
                             .0259040371f,  .7827717662f,   -.8086757660f};
constexpr glm::mat3 kLab2Lms{1.f, 0.3963377774f,  0.2158037573f, 1.f, -0.1055613458f, -.0638541728f,
                             1.f, -0.0894841775f, -1.2914855480f};
constexpr glm::mat3 kLms2Rgb{4.0767416621f,  -3.3077115913f, .2309699292f,
                             -1.2684380046f, 2.6097574011f,  -.3413193965f,
                             -.0041960863f,  -.7034186147f,  1.7076147010f};

inline constexpr cvec3 cmul(const cvec3& v, const glm::mat3& m) {
  return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
          m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
          m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
}

inline constexpr cvec4 rgb2oklab(const cvec4& c) {
  auto lms = cmul(cvec3{c.x, c.y, c.z}, kRgb2Lms);
  if (std::is_constant_evaluated()) {
    lms = cvec3{gcem::pow(lms.x, 1.f / 3), gcem::pow(lms.y, 1.f / 3), gcem::pow(lms.z, 1.f / 3)};
  } else {
    lms = glm::pow(lms, cvec3{1.f / 3});
  }
  auto lab = cmul(lms, kLms2Lab);
  return {lab.x, lab.y, lab.z, c.w};
}

inline constexpr cvec4 oklab2rgb(const cvec4& c) {
  auto lms = cmul(cvec3{c.x, c.y, c.z}, kLab2Lms);
  lms = {lms.x * lms.x * lms.x, lms.y * lms.y * lms.y, lms.z * lms.z * lms.z};
  auto rgb = cmul(lms, kLms2Rgb);
  return {rgb.x, rgb.y, rgb.z, c.w};
}

inline constexpr cvec4 linear_mix(const cvec4& a, const cvec4& b, float t = .5f) {
  auto rgb_a = srgb2rgb(hsl2srgb(a));
  auto rgb_b = srgb2rgb(hsl2srgb(b));
  return srgb2hsl(rgb2srgb((1.f - t) * rgb_a + t * rgb_b));
}

inline constexpr cvec4 perceptual_mix(const cvec4& a, const cvec4& b, float t = .5f) {
  auto oklab_a = rgb2oklab(srgb2rgb(hsl2srgb(a)));
  auto oklab_b = rgb2oklab(srgb2rgb(hsl2srgb(b)));
  return srgb2hsl(rgb2srgb(oklab2rgb((1.f - t) * oklab_a + t * oklab_b)));
}

inline constexpr cvec4 hsl2oklab(const cvec4& hsl) {
  return rgb2oklab(srgb2rgb(hsl2srgb(hsl)));
}

inline constexpr cvec4 hsl2oklab_cycle(const cvec4& hsl, float cycle) {
  auto lab = hsl2oklab(hsl);
  if (std::is_constant_evaluated()) {
    auto c = gcem::sqrt(lab.y * lab.y + lab.z * lab.z);
    float h = gcem::atan2(lab.z, lab.y) + 2.f * pi<float> * cycle;
    return {lab.x, c * gcem::cos(h), c * gcem::sin(h), lab.a};
  }
  auto c = std::sqrt(lab.y * lab.y + lab.z * lab.z);
  float h = std::atan2(lab.z, lab.y) + 2.f * pi<float> * cycle;
  return {lab.x, c * std::cos(h), c * std::sin(h), lab.a};
}

inline constexpr cvec4 hue(float h, float l = .5f, float s = 1.f) {
  return {h, s, l, 1.f};
}

inline constexpr cvec4 hue360(std::uint32_t h, float l = .5f, float s = 1.f) {
  return hue(h / 360.f, l, s);
}

constexpr auto kZero = cvec4{0.f};
constexpr auto kBlack0 = cvec4{0.f, 0.f, 0.f, 1.f};
constexpr auto kBlack1 = cvec4{0.f, 0.f, 1.f / 32, 1.f};
constexpr auto kWhite0 = cvec4{0.f, 0.f, 1.f, 1.f};
constexpr auto kWhite1 = cvec4{0.f, 0.f, .75f, 1.f};
constexpr auto kOutline = kBlack1;

namespace misc {
constexpr auto kNewPurple = hue360(270, .6f, .7f);
constexpr auto kNewGreen0 = hue360(120, .5f, .4f);
}  // namespace misc

namespace ui {
constexpr auto kBlackOverlay0 = cvec4{0.f, 0.f, 0.f, 1.f / 4};
constexpr auto kBlackOverlay1 = cvec4{0.f, 0.f, 0.f, 3.f / 8};
constexpr auto kPanelBorder0 = cvec4{0.f, 0.f, 1.f, 1.f / 2};
}  // namespace ui

namespace solarized {
constexpr auto kDarkBase03 = srgb2hsl(0, 43, 54);
constexpr auto kDarkBase02 = srgb2hsl(7, 54, 66);
constexpr auto kDarkBase01 = srgb2hsl(88, 110, 117);
constexpr auto kDarkBase00 = srgb2hsl(101, 123, 131);
constexpr auto kDarkBase0 = srgb2hsl(131, 148, 150);
constexpr auto kDarkBase1 = srgb2hsl(147, 161, 161);
constexpr auto kDarkBase2 = srgb2hsl(238, 232, 213);
constexpr auto kDarkBase3 = srgb2hsl(253, 246, 227);

constexpr auto kDarkYellow = srgb2hsl(181, 137, 0);
constexpr auto kDarkOrange = srgb2hsl(203, 75, 22);
constexpr auto kDarkRed = srgb2hsl(220, 50, 47);
constexpr auto kDarkMagenta = srgb2hsl(211, 54, 130);
constexpr auto kDarkViolet = srgb2hsl(108, 113, 196);
constexpr auto kDarkBlue = srgb2hsl(38, 139, 210);
constexpr auto kDarkCyan = srgb2hsl(42, 161, 152);
constexpr auto kDarkGreen = srgb2hsl(133, 153, 0);
}  // namespace solarized

namespace category {
constexpr auto kGeneral = solarized::kDarkBase1;
constexpr auto kCorruption = misc::kNewPurple;
constexpr auto kCloseCombat = solarized::kDarkRed;
constexpr auto kLightning = solarized::kDarkCyan;
constexpr auto kSniper = misc::kNewGreen0;
constexpr auto kGravity = solarized::kDarkViolet;
constexpr auto kLaser = solarized::kDarkBlue;
constexpr auto kPummel = solarized::kDarkYellow;
constexpr auto kRemote = solarized::kDarkBase2;
constexpr auto kUnknown0 = kWhite1;  // TODO
constexpr auto kUnknown1 = kWhite1;  // TODO
constexpr auto kDefender = solarized::kDarkBase01;
constexpr auto kCluster = solarized::kDarkOrange;
}  // namespace category

namespace a {
constexpr auto kEffect0 = 1.f / 2;
constexpr auto kFill0 = 1.f / 3;
constexpr auto kFill1 = 1.f / 4;
constexpr auto kFill2 = 1.f / 6;
constexpr auto kShadow0 = 1.f / 6;
constexpr auto kBackground0 = 1.f / 20;
}  // namespace a

namespace z {
constexpr auto kBackgroundEffect0 = -104.f;
constexpr auto kBackgroundEffect1 = -100.f;
constexpr auto kParticleOutline = -98.f;
constexpr auto kParticle = -96.f;
constexpr auto kOutline = -88.f;
constexpr auto kEffect0 = -80.f;
constexpr auto kEffect1 = -72.f;
constexpr auto kTrails = -64.f;
constexpr auto kParticleFlash = -48.f;
constexpr auto kPowerup = -16.f;
constexpr auto kEnemyBoss = -12.f;
constexpr auto kEnemyWall = -8.f;
constexpr auto kEnemyLarge = 0.f;
constexpr auto kEnemyMedium = 4.f;
constexpr auto kEnemySmall = 8.f;
constexpr auto kPlayerShot = 32.f;
constexpr auto kPlayerPowerup = 64.f;
constexpr auto kPlayerBubble = 72.f;
constexpr auto kPlayer = 96.f;
constexpr auto kPlayerCursor = 100.f;
}  // namespace z

}  // namespace ii::colour

#endif
