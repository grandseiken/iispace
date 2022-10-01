#ifndef II_GAME_CORE_SIM_SIM_LAYER_H
#define II_GAME_CORE_SIM_SIM_LAYER_H
#include "game/core/ui/game_stack.h"
#include "game/core/ui/input.h"
#include "game/logic/sim/io/player.h"
#include <memory>
#include <span>

namespace ii {
struct initial_conditions;
struct game_options_t;

class SimLayer : public ui::GameLayer {
public:
  ~SimLayer() override;
  SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
           std::span<const ui::input_device_id> input_devices,
           std::optional<network_input_mapping> network = std::nullopt);

  void update_content(const ui::input_frame&, ui::output_frame&) override;
  void render_content(render::GlRenderer& r) const override;

private:
  std::string network_debug_text(std::uint32_t index);
  void networked_update(std::vector<input_frame>&&);
  void end_game();

  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif
