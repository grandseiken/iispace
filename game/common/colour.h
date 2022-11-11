#ifndef II_GAME_COMMON_COLOURS_H
#define II_GAME_COMMON_COLOURS_H
#include <glm/glm.hpp>

namespace ii::colour {

inline constexpr glm::vec4 alpha(const glm::vec4& c, float a) {
  return {c.x, c.y, c.z, c.w * a};
}

inline constexpr glm::vec4 rgb2hsl(const glm::vec4& c) {
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

inline constexpr glm::vec4 rgb2hsl(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return rgb2hsl({static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f,
                  static_cast<float>(b) / 255.f, 1.f});
}

inline constexpr glm::vec4 hue(float h, float l = .5f, float s = 1.f) {
  return {h, s, l, 1.f};
}

inline constexpr glm::vec4 hue360(std::uint32_t h, float l = .5f, float s = 1.f) {
  return hue(h / 360.f, l, s);
}

constexpr auto kBlack = glm::vec4{0.f, 0.f, 0.f, 1.f};
constexpr auto kWhite = glm::vec4{1.f, 1.f, 1.f, 1.f};

constexpr auto kSolarizedDarkBase03 = rgb2hsl(0, 43, 54);
constexpr auto kSolarizedDarkBase02 = rgb2hsl(7, 54, 66);
constexpr auto kSolarizedDarkBase01 = rgb2hsl(88, 110, 117);
constexpr auto kSolarizedDarkBase00 = rgb2hsl(101, 123, 131);
constexpr auto kSolarizedDarkBase0 = rgb2hsl(131, 148, 150);
constexpr auto kSolarizedDarkBase1 = rgb2hsl(147, 161, 161);
constexpr auto kSolarizedDarkBase2 = rgb2hsl(238, 232, 213);
constexpr auto kSolarizedDarkBase3 = rgb2hsl(253, 246, 227);

constexpr auto kSolarizedDarkYellow = rgb2hsl(181, 137, 0);
constexpr auto kSolarizedDarkOrange = rgb2hsl(203, 75, 22);
constexpr auto kSolarizedDarkRed = rgb2hsl(220, 50, 47);
constexpr auto kSolarizedDarkMagenta = rgb2hsl(211, 54, 130);
constexpr auto kSolarizedDarkViolet = rgb2hsl(108, 113, 196);
constexpr auto kSolarizedDarkBlue = rgb2hsl(38, 139, 210);
constexpr auto kSolarizedDarkCyan = rgb2hsl(42, 161, 152);
constexpr auto kSolarizedDarkGreen = rgb2hsl(133, 153, 0);
}  // namespace ii::colour

#endif
