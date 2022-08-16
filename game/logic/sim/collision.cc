#include "game/logic/sim/collision.h"
#include <algorithm>

namespace ii {

GridCollisionIndex::GridCollisionIndex(const glm::uvec2& cell_dimensions,
                                       const glm::ivec2& min_point, const glm::ivec2& max_point)
: cell_dimensions_{cell_dimensions}
, cell_offset_{cell_coords(min_point)}
, cell_count_{glm::ivec2{1, 1} + cell_coords(max_point) - cell_offset_} {
  cells_.resize(static_cast<std::size_t>(cell_count_.x) * cell_count_.y);
}

void GridCollisionIndex::refresh_handles(ecs::EntityIndex& index) {
  for (auto& pair : entities_) {
    auto& e = pair.second;
    e.handle = *index.get(e.id);
    e.collision = e.handle.get<Collision>();
    e.transform = e.handle.get<Transform>();
  }
}

void GridCollisionIndex::add(ecs::handle& h, const Collision& c) {
  auto [it, _] = entities_.emplace(h.id(), entry_t{h.id(), h, h.get<Transform>(), &c});
  insert_cells(h.id(), it->second);
}

void GridCollisionIndex::update(ecs::handle& h, const Collision&) {
  auto it = entities_.find(h.id());
  if (it == entities_.end()) {
    return;
  }
  auto& e = it->second;
  auto min = min_coords(e.transform->centre - e.collision->bounding_width);
  auto max = max_coords(e.transform->centre + e.collision->bounding_width);
  if (min != e.min || max != e.max) {
    clear_cells(it->first, e);
    insert_cells(it->first, e);
  }
}

void GridCollisionIndex::remove(ecs::handle& h) {
  auto it = entities_.find(h.id());
  if (it == entities_.end()) {
    return;
  }
  clear_cells(it->first, it->second);
  entities_.erase(it);
}

void GridCollisionIndex::begin_tick() {}

bool GridCollisionIndex::any_collision(const vec2& point, shape_flag mask) const {
  auto coords = cell_coords(point);
  if (!is_cell_valid(coords)) {
    return false;
  }
  for (auto id : cell(coords).entries) {
    const auto& e = entities_.find(id)->second;
    const auto& c = *e.collision;
    auto min = e.transform->centre - e.collision->bounding_width;
    auto max = e.transform->centre + e.collision->bounding_width;
    if (max.x < point.x || max.y < point.y || min.x > point.x || min.y > point.y) {
      continue;
    }
    if (+(c.flags & mask) && +c.check(e.handle, point, mask)) {
      return true;
    }
  }
  return false;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collision_list(const vec2& point, shape_flag mask) const {
  std::vector<SimInterface::collision_info> r;
  auto coords = cell_coords(point);
  if (!is_cell_valid(coords)) {
    return r;
  }
  for (auto id : cell(coords).entries) {
    const auto& e = entities_.find(id)->second;
    const auto& c = *e.collision;
    if (!(c.flags & mask)) {
      continue;
    }
    auto min = e.transform->centre - e.collision->bounding_width;
    auto max = e.transform->centre + e.collision->bounding_width;
    if (max.x < point.x || max.y < point.y || min.x > point.x || min.y > point.y) {
      continue;
    }
    if (auto hit = c.check(e.handle, point, mask); + hit) {
      assert(r.empty() || e.handle.id() > r.back().h.id());
      r.emplace_back(SimInterface::collision_info{.h = e.handle, .hit_mask = hit});
    }
  }
  return r;
}

glm::ivec2 GridCollisionIndex::cell_coords(const vec2& v) const {
  return cell_coords(glm::ivec2{v.x.to_int(), v.y.to_int()});
}

glm::ivec2 GridCollisionIndex::cell_coords(const glm::ivec2& v) const {
  return {v.x >= 0 ? v.x / cell_dimensions_.x : -((-v.x - 1) / cell_dimensions_.x) - 1,
          v.y >= 0 ? v.y / cell_dimensions_.y : -((-v.y - 1) / cell_dimensions_.y) - 1};
}

glm::ivec2 GridCollisionIndex::min_coords(const vec2& v) const {
  return glm::max(cell_coords(v), cell_offset_);
}

glm::ivec2 GridCollisionIndex::max_coords(const vec2& v) const {
  return glm::min(cell_coords(v), cell_offset_ + cell_count_ - glm::ivec2{1, 1});
}

bool GridCollisionIndex::is_cell_valid(const glm::ivec2& cell_coords) const {
  auto c = cell_coords - cell_offset_;
  return glm::all(glm::greaterThanEqual(c, glm::ivec2{0, 0})) &&
      glm::all(glm::lessThan(c, cell_count_));
}

auto GridCollisionIndex::cell(const glm::ivec2& cell_coords) const -> const cell_t& {
  auto c = cell_coords - cell_offset_;
  assert(c.y * cell_count_.x + c.x < cells_.size());
  return cells_[c.y * cell_count_.x + c.x];
}

auto GridCollisionIndex::cell(const glm::ivec2& cell_coords) -> cell_t& {
  auto c = cell_coords - cell_offset_;
  assert(c.y * cell_count_.x + c.x < cells_.size());
  return cells_[c.y * cell_count_.x + c.x];
}

void GridCollisionIndex::clear_cells(ecs::entity_id id, entry_t& e) {
  for (std::int32_t y = e.min.y; y <= e.max.y; ++y) {
    for (std::int32_t x = e.min.x; x <= e.max.x; ++x) {
      cell(glm::ivec2{x, y}).clear(id);
    }
  }
}

void GridCollisionIndex::insert_cells(ecs::entity_id id, entry_t& e) {
  e.min = min_coords(e.transform->centre - e.collision->bounding_width);
  e.max = max_coords(e.transform->centre + e.collision->bounding_width);
  for (std::int32_t y = e.min.y; y <= e.max.y; ++y) {
    for (std::int32_t x = e.min.x; x <= e.max.x; ++x) {
      cell(glm::ivec2{x, y}).insert(id);
    }
  }
}

void GridCollisionIndex::cell_t::insert(ecs::entity_id id) {
  auto it = entries.begin();
  while (it != entries.end() && *it < id) {
    ++it;
  }
  entries.insert(it, id);
}

void GridCollisionIndex::cell_t::clear(ecs::entity_id id) {
  entries.erase(std::find(entries.begin(), entries.end(), id));
}

void LegacyCollisionIndex::refresh_handles(ecs::EntityIndex& index) {
  for (auto& e : entries) {
    e.handle = *index.get(e.id);
    e.collision = e.handle.get<Collision>();
    e.transform = e.handle.get<Transform>();
  }
}

void LegacyCollisionIndex::add(ecs::handle& h, const Collision& c) {
  entries.emplace_back(entry{h.id(), h, h.get<Transform>(), &c, 0});
}

void LegacyCollisionIndex::update(ecs::handle& h, const Collision& c) {}

void LegacyCollisionIndex::remove(ecs::handle& h) {
  if (auto it = std::find_if(entries.begin(), entries.end(),
                             [&](const auto& e) { return e.handle.id() == h.id(); });
      it != entries.end()) {
    entries.erase(it);
  }
}

void LegacyCollisionIndex::begin_tick() {
  for (auto& e : entries) {
    e.x_min = e.transform->centre.x - e.collision->bounding_width;
  }
  std::stable_sort(entries.begin(), entries.end(),
                   [](const auto& a, const auto& b) { return a.x_min < b.x_min; });
}

bool LegacyCollisionIndex::any_collision(const vec2& point, shape_flag mask) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : entries) {
    const auto& e = *collision.collision;
    auto v = collision.transform->centre;
    fixed w = e.bounding_width;

    // This optmization check is incorrect, since collision list is sorted based on x-min
    // at start of frame; may have moved in the meantime, or list may have been appended to.
    // Very unclear what effective legacy collision semantics actually are.
    // This is fixed in the replacement collision index.
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }
    if (!(e.flags & mask)) {
      continue;
    }
    if (+e.check(collision.handle, point, mask)) {
      return true;
    }
  }
  return false;
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collision_list(const vec2& point, shape_flag mask) const {
  std::vector<SimInterface::collision_info> r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : entries) {
    const auto& e = *collision.collision;
    auto v = collision.transform->centre;
    fixed w = e.bounding_width;

    // Same issues as in any_collision().
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }
    if (!(e.flags & mask)) {
      continue;
    }
    if (auto hit = e.check(collision.handle, point, mask); + hit) {
      r.emplace_back(SimInterface::collision_info{.h = collision.handle, .hit_mask = hit});
    }
  }
  return r;
}

}  // namespace ii