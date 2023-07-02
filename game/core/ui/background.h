#ifndef II_GAME_CORE_UI_BACKGROUND_H
#define II_GAME_CORE_UI_BACKGROUND_H
#include "game/common/random.h"
#include "game/render/data/background.h"
#include <vector>

namespace ii::ui {

class BackgroundState {
public:
  BackgroundState(RandomEngine&);
  void handle(const render::background::update&);
  void update();
  const render::background& output() const { return output_; }

private:
  std::vector<render::background::update> update_queue_;
  std::uint32_t interpolate_ = 0;
  render::background output_;
};

}  // namespace ii::ui

#endif
