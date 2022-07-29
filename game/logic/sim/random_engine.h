#ifndef II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#define II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#include "game/common/fix32.h"
#include <cstdint>

namespace ii {

class RandomEngine {
public:
  static constexpr std::uint32_t rand_max = 0x7fff;

  RandomEngine(std::uint32_t seed) : state_{seed} {}

  std::uint32_t state() const {
    return state_;
  }

  std::uint32_t operator()() {
    // TODO: ... lol?
    static constexpr std::uint32_t a = 1103515245;
    static constexpr std::uint32_t c = 12345;
    state_ = a * state_ + c;
    return (state_ >> 16) & rand_max;
  }

  std::uint32_t uint(std::uint32_t max) {
    return (*this)() % max;
  }

  ::fixed fixed() {
    return ::fixed{static_cast<std::int32_t>((*this)())} / RandomEngine::rand_max;
  }

  bool rbool() {
    return uint(2);
  }

private:
  std::uint32_t state_ = 0;
};

}  // namespace ii

#endif