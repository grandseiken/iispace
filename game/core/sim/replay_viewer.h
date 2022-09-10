#ifndef II_GAME_CORE_SIM_SIM_LAYER_H
#define II_GAME_CORE_SIM_SIM_LAYER_H
#include "game/core/ui/game_stack.h"
#include <memory>

namespace ii::data {
class ReplayReader;
}  // namespace ii::data

namespace ii {

class ReplayViewer : public ui::GameLayer {
public:
  ~ReplayViewer() override;
  ReplayViewer(ui::GameStack& stack, data::ReplayReader&& replay);

  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer& r) const override;

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif
