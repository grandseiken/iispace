#include "game/core/render_state.h"
#include "game/core/io_input_adapter.h"
#include "game/logic/sim/sim_state.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <unordered_map>

namespace ii {

void RenderState::handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input) {
  auto& output = state.output();

  // Particles.
  particles_.insert(particles_.end(), output.particles.begin(), output.particles.end());
  output.particles.clear();

  // Rumble.
  if (input) {
    for (const auto& pair : output.rumble) {
      // Rumble currently defined in ticks. TODO: plumb through lf/hf.
      auto duration_ms = static_cast<std::uint32_t>(pair.second * 16);
      input->rumble(pair.first, 0x7fff, 0x7fff, duration_ms);
    }
    output.rumble.clear();
  }

  // Sounds.
  if (mixer) {
    struct sound_average {
      std::size_t count = 0;
      float volume = 0.f;
      float pan = 0.f;
      float pitch = 0.f;
    };

    std::unordered_map<sound, sound_average> sound_map;
    for (const auto& entry : output.sounds) {
      auto& e = sound_map[entry.sound_id];
      ++e.count;
      e.volume += entry.volume;
      e.pan += entry.pan;
      e.pitch = entry.pitch;
    }

    for (auto& pair : sound_map) {
      auto& e = pair.second;
      e.volume = std::max(0.f, std::min(1.f, e.volume));
      e.pan = e.pan / static_cast<float>(pair.second.count);
      e.pitch = std::pow(2.f, e.pitch);
      mixer->play(static_cast<ii::Mixer::audio_handle_t>(pair.first), e.volume, e.pan, e.pitch);
    }
    output.sounds.clear();
  }
}

void RenderState::update() {
  for (auto& particle : particles_) {
    if (particle.timer) {
      particle.position += particle.velocity;
      --particle.timer;
    }
  }
  std::erase_if(particles_, [](const particle& p) { return !p.timer; });
}

void RenderState::render(render::GlRenderer& r) const {
  std::vector<render::line_t> lines;
  auto render_line_rect = [&](const glm::vec2& lo, const glm::vec2& hi, const glm::vec4& c) {
    glm::vec2 li{lo.x, hi.y};
    glm::vec2 ho{hi.x, lo.y};
    lines.emplace_back(render::line_t{lo, li, c});
    lines.emplace_back(render::line_t{li, hi, c});
    lines.emplace_back(render::line_t{hi, ho, c});
    lines.emplace_back(render::line_t{ho, lo, c});
  };
  for (const auto& particle : particles_) {
    render_line_rect(particle.position + glm::vec2{1, 1}, particle.position - glm::vec2{1, 1},
                     particle.colour);
  }
  r.render_lines(lines);
}

}  // namespace ii