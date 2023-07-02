#include "game/core/ui/background.h"
#include "game/common/easing.h"
#include <glm/glm.hpp>

namespace ii::ui {
namespace {
constexpr float kAngularVelocityMultiplier = 1.f / 128;
constexpr std::uint32_t kBackgroundInterpolateTime = 180;
}  // namespace

BackgroundState::BackgroundState(RandomEngine& engine) {
  auto v = [&engine] { return engine.fixed().to_float() * 1024.f - 512.f; };
  output_.position.x = v();
  output_.position.y = v();
}

void BackgroundState::handle(const render::background::update& update) {
  update_queue_.emplace_back(update);
  if (update_queue_.size() == 1u) {
    update_queue_.back().fill_defaults();
  } else {
    update_queue_.back().fill_from(*(update_queue_.rbegin() + 1));
  }
}

void BackgroundState::update() {
  if (update_queue_.size() > 1 && ++interpolate_ == kBackgroundInterpolateTime) {
    update_queue_.erase(update_queue_.begin());
    interpolate_ = 0;
  }

  auto set_data = [&](render::background::data& data, const render::background::update& update) {
    data.height_function = *update.height_function;
    data.combinator = *update.combinator;
    data.tonemap = *update.tonemap;

    data.polar_period = *update.polar_period;
    data.scale = *update.scale;
    data.persistence = *update.persistence;
    data.parameters = *update.parameters;
    data.colour = *update.colour;
  };

  render::background::update u0;
  if (update_queue_.empty()) {
    u0.fill_defaults();
  } else {
    u0 = update_queue_[0];
  }
  set_data(output_.data0, u0);

  output_.interpolate =
      ease_in_out_cubic(static_cast<float>(interpolate_) / kBackgroundInterpolateTime);
  fvec4 delta{0.f};
  float rotation_delta = 0.f;
  if (update_queue_.size() > 1) {
    set_data(output_.data1, update_queue_[1]);
    delta = glm::mix(*u0.velocity, *update_queue_[1].velocity, output_.interpolate);
    rotation_delta =
        glm::mix(*u0.angular_velocity, *update_queue_[1].angular_velocity, output_.interpolate);
  } else {
    delta = *u0.velocity;
    rotation_delta = *u0.angular_velocity;
  }
  output_.position += fvec4{1.f, 1.f, 1.f, kAngularVelocityMultiplier} * delta;
  output_.rotation += kAngularVelocityMultiplier * rotation_delta;
  output_.rotation = normalise_angle(output_.rotation);
}

}  // namespace ii::ui