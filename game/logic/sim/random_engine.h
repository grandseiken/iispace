#ifndef II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#define II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#include <cstdint>

namespace ii {

class RandomEngine {
public:
  static constexpr std::uint32_t rand_max = 0x7fff;
  RandomEngine(std::uint32_t seed);
  std::uint32_t operator()();
  std::uint32_t state() const {
    return state_;
  }

private:
  std::uint32_t state_ = 0;
};

}  // namespace ii

#endif