#include "game/core/render_state.h"
#include "game/core/io_input_adapter.h"
#include "game/logic/sim/sim_state.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <unordered_map>

namespace ii {

void RenderState::handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input) {
  struct sound_average {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };

  std::unordered_map<std::uint32_t, std::uint32_t> rumble;
  std::unordered_map<sound, sound_average> sound_map;

  auto& output = state.output();
  for (auto it = output.entries.begin(); it != output.entries.end();) {
    auto& e = it->e;

    // Particles.
    particles_.insert(particles_.end(), e.particles.begin(), e.particles.end());
    e.particles.clear();

    // Rumble.
    if (input) {
      for (const auto& pair : e.rumble_map) {
        rumble[pair.first] = std::max(rumble[pair.first], pair.second);
      }
      for (std::uint32_t i = 0; e.global_rumble && i < input->player_count(); ++i) {
        rumble[i] = std::max(rumble[i], e.global_rumble);
      }
    }
    e.rumble_map.clear();  // Always clear rumble, since either handling it now or never.
    e.global_rumble = 0;

    // Sounds.
    if (mixer) {
      for (const auto& entry : e.sounds) {
        auto& s = sound_map[entry.sound_id];
        ++s.count;
        s.volume += entry.volume;
        s.pan += entry.pan;
        s.pitch = entry.pitch;
      }
      e.sounds.clear();
    }

    if (e.particles.empty() && e.rumble_map.empty() && !e.global_rumble && e.sounds.empty()) {
      it = output.entries.erase(it);
    } else {
      ++it;
    }
  }

  // Final resolution.
  for (const auto& pair : rumble) {
    // Rumble currently defined in ticks. TODO: plumb through lf/hf.
    auto duration_ms = static_cast<std::uint32_t>(pair.second * 16);
    input->rumble(pair.first, 0x7fff, 0x7fff, duration_ms);
  }

  for (auto& pair : sound_map) {
    auto& e = pair.second;
    e.volume = std::max(0.f, std::min(1.f, e.volume));
    e.pan = e.pan / static_cast<float>(pair.second.count);
    e.pitch = std::pow(2.f, e.pitch);
    mixer->play(static_cast<ii::Mixer::audio_handle_t>(pair.first), e.volume, e.pan, e.pitch);
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