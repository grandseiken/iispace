#ifndef II_GAME_COMMON_COLOURS_H
#define II_GAME_COMMON_COLOURS_H
#include <gcem.hpp>
#include <glm/glm.hpp>
#include <algorithm>

namespace ii::colour {

inline constexpr glm::vec4 clamp(const glm::vec4& c) {
  return {std::clamp(c.x, 0.f, 1.f), std::clamp(c.y, 0.f, 1.f), std::clamp(c.z, 0.f, 1.f),
          std::clamp(c.w, 0.f, 1.f)};
}

inline constexpr glm::vec4 alpha(const glm::vec4& c, float a) {
  return clamp({c.x, c.y, c.z, c.w * a});
}

inline constexpr glm::vec4 srgb2rgb(const glm::vec4& c) {
  auto f = [](float v) {
    return v >= .04045f ? gcem::pow((v + .055f) / 1.055f, 2.4f) : v / 12.92f;
  };
  return {f(c.x), f(c.y), f(c.z), c.w};
}

inline constexpr glm::vec4 rgb2srgb(const glm::vec4& c) {
  auto f = [](float v) {
    return v >= .0031308f ? 1.055f * gcem::pow(v, 1.f / 2.4f) - .055f : v * 12.92f;
  };
  return {f(c.x), f(c.y), f(c.z), c.w};
}

inline constexpr glm::vec4 hsl2srgb(const glm::vec4& c) {
  float h = gcem::fmod(c.x, 1.f);
  float s = c.y;
  float l = c.z;
  float v = l <= .5f ? l * (1.f + s) : l + s - l * s;

  if (v == 0.f) {
    return {0.f, 0.f, 0.f, c.w};
  }
  float m = 2.f * l - v;
  float sv = (v - m) / v;
  float sextant = h * 6.f;
  float vsf = v * sv * gcem::fmod(h * 6.f, 1.f);
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

inline constexpr glm::vec4 srgb2hsl(const glm::vec4& c) {
  auto v = std::max(c.x, std::max(c.y, c.z));
  auto m = std::min(c.x, std::min(c.y, c.z));
  auto l = (m + v) / 2.f;
  if (l <= 0.f) {
    return glm::vec4{0.f, 0.f, 0.f, c.w};
  }
  auto vm = v - m;
  auto s = vm;
  if (s <= 0.f) {
    return glm::vec4{0.f, 0.f, l, c.w};
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

inline constexpr glm::vec4 srgb2hsl(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return srgb2hsl({static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f,
                   static_cast<float>(b) / 255.f, 1.f});
}

inline constexpr glm::vec4 hsl_mix(const glm::vec4& a, const glm::vec4& b, float t = .5f) {
  auto rgb_a = srgb2rgb(hsl2srgb(a));
  auto rgb_b = srgb2rgb(hsl2srgb(b));
  return srgb2hsl(rgb2srgb((1.f - t) * rgb_a + t * rgb_b));
}

inline constexpr glm::vec4 hue(float h, float l = .5f, float s = 1.f) {
  return {h, s, l, 1.f};
}

inline constexpr glm::vec4 hue360(std::uint32_t h, float l = .5f, float s = 1.f) {
  return hue(h / 360.f, l, s);
}

constexpr auto kZero = glm::vec4{0.f};
constexpr auto kBlack1 = glm::vec4{0.f, 0.f, 1.f / 32, 1.f};
constexpr auto kBlack0 = glm::vec4{0.f, 0.f, 0.f, 1.f};
constexpr auto kWhite0 = glm::vec4{0.f, 0.f, 1.f, 1.f};
constexpr auto kWhite1 = glm::vec4{0.f, 0.f, .75f, 1.f};

constexpr auto kSolarizedDarkBase03 = srgb2hsl(0, 43, 54);
constexpr auto kSolarizedDarkBase02 = srgb2hsl(7, 54, 66);
constexpr auto kSolarizedDarkBase01 = srgb2hsl(88, 110, 117);
constexpr auto kSolarizedDarkBase00 = srgb2hsl(101, 123, 131);
constexpr auto kSolarizedDarkBase0 = srgb2hsl(131, 148, 150);
constexpr auto kSolarizedDarkBase1 = srgb2hsl(147, 161, 161);
constexpr auto kSolarizedDarkBase2 = srgb2hsl(238, 232, 213);
constexpr auto kSolarizedDarkBase3 = srgb2hsl(253, 246, 227);

constexpr auto kSolarizedDarkYellow = srgb2hsl(181, 137, 0);
constexpr auto kSolarizedDarkOrange = srgb2hsl(203, 75, 22);
constexpr auto kSolarizedDarkRed = srgb2hsl(220, 50, 47);
constexpr auto kSolarizedDarkMagenta = srgb2hsl(211, 54, 130);
constexpr auto kSolarizedDarkViolet = srgb2hsl(108, 113, 196);
constexpr auto kSolarizedDarkBlue = srgb2hsl(38, 139, 210);
constexpr auto kSolarizedDarkCyan = srgb2hsl(42, 161, 152);
constexpr auto kSolarizedDarkGreen = srgb2hsl(133, 153, 0);

constexpr auto kBackgroundAlpha0 = 1.f / (256 + 128);
constexpr auto kFillAlpha0 = 1.f / 16;
constexpr auto kFillAlpha1 = 1.f / 32;
constexpr auto kOutline = kBlack1;
constexpr auto kNewPurple = hue360(270, .6f, .7f);
constexpr auto kNewGreen0 = hue360(120, .5f, .4f);

constexpr auto kZBackgroundEffect = -100.f;
constexpr auto kZParticle = -96.f;
constexpr auto kZOutline = -88.f;
constexpr auto kZEffect0 = -80.f;
constexpr auto kZEffect1 = -72.f;
constexpr auto kZTrails = -64.f;
constexpr auto kZParticleFlash = -48.f;
constexpr auto kZPowerup = -16.f;
constexpr auto kZEnemyWall = -8.f;
constexpr auto kZEnemyLarge = 0.f;
constexpr auto kZEnemyMedium = 4.f;
constexpr auto kZEnemySmall = 8.f;
constexpr auto kZPlayerShot = 32.f;
constexpr auto kZPlayerPowerup = 64.f;
constexpr auto kZPlayerBubble = 72.f;
constexpr auto kZPlayer = 96.f;
}  // namespace ii::colour

#endif
