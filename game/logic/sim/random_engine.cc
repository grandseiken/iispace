#include "game/logic/sim/random_engine.h"

namespace ii {

RandomEngine::RandomEngine(std::uint32_t seed) : state_{seed} {}

std::uint32_t RandomEngine::operator()() {
  // TODO: ... lol?
  static constexpr std::uint32_t a = 1103515245;
  static constexpr std::uint32_t c = 12345;
  state_ = a * state_ + c;
  return (state_ >> 16) & rand_max;
}

}  // namespace ii