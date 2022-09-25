#ifndef II_GAME_SYSTEM_SYSTEM_H
#define II_GAME_SYSTEM_SYSTEM_H
#include "game/common/result.h"
#include "game/common/ustring.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ii {

class System {
public:
  virtual ~System() = default;
  virtual result<std::vector<std::string>> init() = 0;

  virtual bool supports_network_multiplayer() const = 0;
  virtual ustring local_username() const = 0;
  virtual std::size_t friends_in_game() const = 0;
};

std::unique_ptr<System> create_system();

}  // namespace ii

#endif