#ifndef II_GAME_SYSTEM_DEFAULT_H
#define II_GAME_SYSTEM_DEFAULT_H
#include "game/system/system.h"

namespace ii {

class DefaultSystem : public System {
public:
  ~DefaultSystem() override = default;

  result<std::vector<std::string>> init() override {
    return {};
  }
};

}  // namespace ii

#endif