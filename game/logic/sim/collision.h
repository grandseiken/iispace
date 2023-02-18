#ifndef GAME_LOGIC_SIM_COLLISION_H
#define GAME_LOGIC_SIM_COLLISION_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/components.h"
#include "game/logic/sim/sim_interface.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace ii {

class CollisionIndex {
public:
  virtual ~CollisionIndex() = default;

  virtual std::unique_ptr<CollisionIndex> clone() = 0;
  virtual void refresh_handles(ecs::EntityIndex& index) = 0;
  virtual void add(ecs::handle& h, const Collision& c) = 0;
  virtual void update(ecs::handle& h) = 0;
  virtual void remove(ecs::handle& h) = 0;
  virtual void begin_tick() = 0;

  virtual bool collide_any(const geom::check_t&) const = 0;
  virtual std::vector<SimInterface::collision_info> collide(const geom::check_t&) const = 0;
  virtual void in_range(const vec2& point, fixed distance, ecs::component_id, std::size_t max_n,
                        std::vector<SimInterface::range_info>& output) const = 0;
};

// TODO: probably needs optimizing.
class GridCollisionIndex : public CollisionIndex {
public:
  GridCollisionIndex(const uvec2& cell_dimensions, const ivec2& min_point, const ivec2& max_point);

  ~GridCollisionIndex() override = default;
  std::unique_ptr<CollisionIndex> clone() override {
    return std::make_unique<GridCollisionIndex>(*this);
  }

  void refresh_handles(ecs::EntityIndex& index) override;
  void add(ecs::handle& h, const Collision& c) override;
  void update(ecs::handle& h) override;
  void remove(ecs::handle& h) override;
  void begin_tick() override;

private:
  template <typename F>
  void iterate_collision_cells(const geom::check_t&, const F&) const;

public:
  bool collide_any(const geom::check_t&) const override;
  std::vector<SimInterface::collision_info> collide(const geom::check_t&) const override;
  void in_range(const vec2& point, fixed distance, ecs::component_id, std::size_t max_n,
                std::vector<SimInterface::range_info>& output) const override;

private:
  struct cell_t;
  struct entry_t;

  ivec2 cell_position(const ivec2& c) const;
  ivec2 cell_coords(const ivec2& v) const;
  ivec2 cell_coords(const vec2& v) const;
  ivec2 max_coords(const vec2& v) const;
  ivec2 min_coords(const vec2& v) const;
  bool is_cell_valid(const ivec2& cell_coords) const;
  const cell_t& cell(const ivec2& cell_coords) const;
  cell_t& cell(const ivec2& cell_coords);

  void clear_cells(ecs::entity_id id, entry_t& e);
  void insert_cells(ecs::entity_id id, entry_t& e);

  struct cell_t {
    void insert(ecs::entity_id id);
    void clear(ecs::entity_id id);
    void insert_centre(ecs::entity_id id);
    void clear_centre(ecs::entity_id id);
    std::vector<ecs::entity_id> entries;
    std::vector<ecs::entity_id> centres;
  };

  struct entry_t {
    ecs::entity_id id;
    ecs::handle handle;
    const Transform* transform = nullptr;
    const Collision* collision = nullptr;
    ivec2 min{0, 0};
    ivec2 max{0, 0};
    ivec2 centre{0, 0};
  };

  ivec2 cell_power_{0, 0};
  ivec2 cell_offset_{0, 0};
  ivec2 cell_count_{0, 0};
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
  void update(ecs::handle& h) override;
  void remove(ecs::handle& h) override;
  void begin_tick() override;

  bool collide_any(const geom::check_t&) const override;
  std::vector<SimInterface::collision_info> collide(const geom::check_t&) const override;
  void in_range(const vec2& point, fixed distance, ecs::component_id, std::size_t max_n,
                std::vector<SimInterface::range_info>& output) const override;

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
