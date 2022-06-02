#ifndef II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#define II_GAME_LOGIC_SIM_RANDOM_ENGINE_H
#include <cstdint>

namespace ii {

class RandomEngine {
public:
  static constexpr std::int32_t rand_max = 0x7fff;
  RandomEngine(std::int32_t seed);
  std::int32_t operator()();

private:
  std::int32_t state_ = 0;
};

}  // namespace ii

#endif