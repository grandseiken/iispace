#ifndef II_GAME_SYSTEM_SYSTEM_H
#define II_GAME_SYSTEM_SYSTEM_H
#include "game/common/result.h"
#include <memory>
#include <string>
#include <vector>

namespace ii {

class System {
public:
  virtual ~System() = default;
  virtual result<std::vector<std::string>> init() = 0;
};

std::unique_ptr<System> create_system();

}  // namespace ii

#endif