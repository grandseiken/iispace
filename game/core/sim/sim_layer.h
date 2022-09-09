#ifndef II_GAME_CORE_SIM_SIM_LAYER_H
#define II_GAME_CORE_SIM_SIM_LAYER_H
#include "game/core/ui/game_stack.h"
#include <memory>

namespace ii::data {
class ReplayReader;
}  // namespace ii::data

namespace ii {
struct initial_conditions;
struct game_options_t;

class SimLayer : public ui::GameLayer {
public:
  ~SimLayer() override;
  SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
           const game_options_t& options);
  SimLayer(ui::GameStack& stack, data::ReplayReader&& replay, const game_options_t& options);

  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer& r) const override;

private:
  void end_game();

  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif
