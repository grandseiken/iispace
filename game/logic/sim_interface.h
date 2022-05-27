#ifndef IISPACE_GAME_LOGIC_SIM_INTERFACE_H
#define IISPACE_GAME_LOGIC_SIM_INTERFACE_H
#include <cstdint>

class Lib;
class SimState;

namespace ii {
constexpr std::int32_t kSimWidth = 640;
constexpr std::int32_t kSimHeight = 480;

class SimInterface {
public:
  SimInterface(Lib& lib, SimState& state) : lib_{lib}, state_{state} {}
  Lib& lib() {
    return lib_;
  }
  SimState& state() {
    return state_;
  }

private:
  Lib& lib_;
  SimState& state_;
};

}  // namespace ii

#endif