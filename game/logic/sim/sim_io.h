#ifndef II_GAME_LOGIC_SIM_SIM_IO_H
#define II_GAME_LOGIC_SIM_SIM_IO_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {

static constexpr std::uint32_t kMaxPlayers = 4;

inline glm::vec4 player_colour(std::size_t player_number) {
  return player_number == 0 ? colour_hue360(0)
      : player_number == 1  ? colour_hue360(20)
      : player_number == 2  ? colour_hue360(40)
      : player_number == 3  ? colour_hue360(60)
                            : colour_hue360(120);
}

enum class game_mode : std::uint32_t {
  kNormal,
  kBoss,
  kHard,
  kFast,
  kWhat,
  kMax,
};

enum class boss_flag : std::uint32_t {
  kBoss1A = 1,
  kBoss1B = 2,
  kBoss1C = 4,
  kBoss2A = 8,
  kBoss2B = 16,
  kBoss2C = 32,
  kBoss3A = 64,
};

template <>
struct bitmask_enum<boss_flag> : std::true_type {};

struct initial_conditions {
  std::uint32_t seed = 0;
  std::uint32_t player_count = 0;
  game_mode mode = game_mode::kNormal;
  bool can_face_secret_boss = false;
};

struct input_frame {
  enum key : std::uint32_t {
    kFire = 1,
    kBomb = 2,
  };
  vec2 velocity{0};
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::uint32_t keys = 0;
};

class InputAdapter {
public:
  virtual ~InputAdapter() = default;
  virtual std::vector<input_frame>& get() = 0;
  virtual void next() = 0;
};

struct sound_out {
  float volume = 0.f;
  float pan = 0.f;
  float pitch = 0.f;
};

struct render_output {
  struct player_info {
    glm::vec4 colour{0.f};
    std::uint64_t score = 0;
    std::uint32_t multiplier = 0;
    float timer = 0.f;
  };
  struct line_t {
    glm::vec2 a{0.f};
    glm::vec2 b{0.f};
    glm::vec4 c{0.f};
  };
  std::vector<player_info> players;
  std::vector<line_t> lines;
  game_mode mode = game_mode::kNormal;
  std::uint32_t elapsed_time = 0;
  std::uint32_t lives_remaining = 0;
  std::optional<std::uint32_t> overmind_timer;
  std::optional<float> boss_hp_bar;
  std::uint32_t colour_cycle = 0;
};

struct sim_results {
  game_mode mode = game_mode::kNormal;
  std::uint32_t seed = 0;
  std::uint32_t elapsed_time = 0;
  std::uint32_t killed_bosses = 0;
  std::uint32_t lives_remaining = 0;

  boss_flag bosses_killed{0};
  boss_flag hard_mode_bosses_killed{0};

  struct player_result {
    std::uint32_t number = 0;
    std::uint64_t score = 0;
    std::uint32_t deaths = 0;
  };
  std::vector<player_result> players;
  std::uint64_t score = 0;
};

}  // namespace ii

#endif