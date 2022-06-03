#ifndef II_GAME_LOGIC_SIM_SIM_IO_H
#define II_GAME_LOGIC_SIM_SIM_IO_H
#include "game/common/z.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {

static constexpr std::int32_t kMaxPlayers = 4;

enum class game_mode {
  kNormal,
  kBoss,
  kHard,
  kFast,
  kWhat,
  kMax,
};

struct initial_conditions {
  std::int32_t seed = 0;
  game_mode mode = game_mode::kNormal;
  std::int32_t player_count = 0;
  bool can_face_secret_boss = false;
};

struct input_frame {
  enum key : std::int32_t {
    kFire = 1,
    kBomb = 2,
  };
  vec2 velocity{0};
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::int32_t keys = 0;
};

class InputAdapter {
public:
  virtual ~InputAdapter() {}
  virtual std::vector<input_frame> get() = 0;
};

struct sound_out {
  float volume = 0.f;
  float pan = 0.f;
  float pitch = 0.f;
};

struct render_output {
  struct player_info {
    colour_t colour = 0;
    std::int64_t score = 0;
    std::int32_t multiplier = 0;
    float timer = 0.f;
  };
  struct line_t {
    glm::vec2 a{0.f};
    glm::vec2 b{0.f};
    colour_t c = 0;
  };
  std::vector<player_info> players;
  std::vector<line_t> lines;
  game_mode mode = game_mode::kNormal;
  std::int32_t elapsed_time = 0;
  std::int32_t lives_remaining = 0;
  std::int32_t overmind_timer = 0;
  std::optional<float> boss_hp_bar;
  std::int32_t colour_cycle = 0;
};

struct sim_results {
  game_mode mode = game_mode::kNormal;
  std::int32_t seed = 0;
  std::int32_t elapsed_time = 0;
  std::int32_t killed_bosses = 0;
  std::int32_t lives_remaining = 0;

  std::int32_t bosses_killed = 0;
  std::int32_t hard_mode_bosses_killed = 0;

  struct player_result {
    std::int32_t number = 0;
    std::int64_t score = 0;
    std::int32_t deaths = 0;
  };
  std::vector<player_result> players;
};

}  // namespace ii

#endif