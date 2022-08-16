#ifndef GAME_LOGIC_SIM_COLLISION_H
#define GAME_LOGIC_SIM_COLLISION_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {

class CollisionIndex {
public:
  virtual ~CollisionIndex() = default;

  virtual std::unique_ptr<CollisionIndex> clone() = 0;
  virtual void refresh_handles(ecs::EntityIndex& index) = 0;
  virtual void add(ecs::handle& h, const Collision& c) = 0;
  virtual void update(ecs::handle& h, const Collision& c) = 0;
  virtual void remove(ecs::handle& h) = 0;
  virtual void begin_tick() = 0;

  virtual bool any_collision(const vec2& point, shape_flag mask) const = 0;
  virtual std::vector<SimInterface::collision_info>
  collision_list(const vec2& point, shape_flag mask) const = 0;
};

// TODO: probably needs optimizing.
class GridCollisionIndex : public CollisionIndex {
public:
  GridCollisionIndex(const glm::uvec2& cell_dimensions, const glm::ivec2& min_point,
                     const glm::ivec2& max_point);

  ~GridCollisionIndex() override = default;
  std::unique_ptr<CollisionIndex> clone() override {
    return std::make_unique<GridCollisionIndex>(*this);
  }

  void refresh_handles(ecs::EntityIndex& index) override;
  void add(ecs::handle& h, const Collision& c) override;
  void update(ecs::handle& h, const Collision& c) override;
  void remove(ecs::handle& h) override;
  void begin_tick() override;

  bool any_collision(const vec2& point, shape_flag mask) const override;
  std::vector<SimInterface::collision_info>
  collision_list(const vec2& point, shape_flag mask) const override;

private:
  struct cell_t;
  struct entry_t;

  glm::ivec2 cell_coords(const glm::ivec2& v) const;
  glm::ivec2 cell_coords(const vec2& v) const;
  glm::ivec2 max_coords(const vec2& v) const;
  glm::ivec2 min_coords(const vec2& v) const;
  bool is_cell_valid(const glm::ivec2& cell_coords) const;
  const cell_t& cell(const glm::ivec2& cell_coords) const;
  cell_t& cell(const glm::ivec2& cell_coords);

  void clear_cells(ecs::entity_id id, entry_t& e);
  void insert_cells(ecs::entity_id id, entry_t& e);

  struct cell_t {
    void insert(ecs::entity_id id);
    void clear(ecs::entity_id id);
    std::vector<ecs::entity_id> entries;
  };

  struct entry_t {
    ecs::entity_id id;
    ecs::handle handle;
    const Transform* transform = nullptr;
    const Collision* collision = nullptr;
    glm::ivec2 min{0, 0};
    glm::ivec2 max{0, 0};
  };

  glm::ivec2 cell_power_{0, 0};
  glm::ivec2 cell_offset_{0, 0};
  glm::ivec2 cell_count_{0, 0};
  std::vector<cell_t> cells_;
  std::unordered_map<ecs::entity_id, entry_t> entities_;
};

// Buggy, legacy collision system for use with legacy compatibility mode.
class LegacyCollisionIndex : public CollisionIndex {
public:
  ~LegacyCollisionIndex() override = default;
  std::unique_ptr<CollisionIndex> clone() override {
    return std::make_unique<LegacyCollisionIndex>(*this);
  }

  void refresh_handles(ecs::EntityIndex& index) override;
  void add(ecs::handle& h, const Collision& c) override;
  void update(ecs::handle& h, const Collision& c) override;
  void remove(ecs::handle& h) override;
  void begin_tick() override;

  bool any_collision(const vec2& point, shape_flag mask) const override;
  std::vector<SimInterface::collision_info>
  collision_list(const vec2& point, shape_flag mask) const override;

private:
  struct entry {
    ecs::entity_id id;
    ecs::handle handle;
    const Transform* transform = nullptr;
    const Collision* collision = nullptr;
    fixed x_min = 0;
  };
  std::vector<entry> entries;
};

}  // namespace ii

#endif
