#ifndef II_GAME_COMMON_RANDOM_H
#define II_GAME_COMMON_RANDOM_H
#include "game/common/fix32.h"
#include <concepts>
#include <cstdint>

namespace ii {

class RandomEngine {
public:
  static constexpr std::uint32_t rand_max = 0x7fff;

  RandomEngine(std::uint32_t seed) : state_{seed} {}
  RandomEngine(const RandomEngine&) = delete;
  RandomEngine& operator=(const RandomEngine&) = delete;

  std::uint32_t state() const { return state_; }
  void set_state(std::uint32_t state) { state_ = state; }

  std::uint32_t operator()() {
    // TODO: ... lol?
    static constexpr std::uint32_t a = 1103515245;
    static constexpr std::uint32_t c = 12345;
    state_ = a * state_ + c;
    return (state_ >> 16) & rand_max;
  }

  std::uint32_t uint(std::uint32_t max) { return (*this)() % max; }
  ::fixed fixed() { return ::fixed{static_cast<std::int32_t>((*this)())} / RandomEngine::rand_max; }
  bool rbool() { return uint(2); }

  template <typename It>
  void shuffle(It begin, It end) {
    using std::swap;
    auto size = static_cast<std::uint32_t>(end - begin);
    for (std::uint32_t i = 0; i + 1 < size; ++i) {
      auto j = i + uint(size - i);
      swap(*(begin + i), *(begin + j));
    }
  }

  template <typename It>
  It choice(It begin, It end) {
    return begin == end ? end : begin + uint(static_cast<std::uint32_t>(end - begin));
  }

  template <typename It, typename WeightFunction>
  It weighted_choice(It begin, It end, const WeightFunction& f) {
    std::uint32_t total_weight = 0;
    for (auto it = begin; it != end; ++it) {
      const auto& x = *it;
      total_weight += f(x);
    }
    if (!total_weight) {
      return end;
    }
    auto r = uint(total_weight);
    total_weight = 0;
    for (auto it = begin; it != end; ++it) {
      const auto& x = *it;
      total_weight += f(x);
      if (r < total_weight) {
        return it;
      }
    }
    return end;
  }

private:
  std::uint32_t state_ = 0;
};

}  // namespace ii

#endif