#include "game/logic/sim/collision.h"
#include "game/common/collision.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <unordered_set>

namespace ii {

GridCollisionIndex::GridCollisionIndex(const uvec2& cell_dimensions, const ivec2& min_point,
                                       const ivec2& max_point)
: cell_power_{std::bit_width(std::bit_ceil(cell_dimensions.x)),
              std::bit_width(std::bit_ceil(cell_dimensions.y))}
, cell_offset_{cell_coords(min_point)}
, cell_count_{ivec2{1, 1} + cell_coords(max_point) - cell_offset_} {
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

void GridCollisionIndex::update(ecs::handle& h) {
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

bool GridCollisionIndex::collide_point_any(const vec2& point, shape_flag mask) const {
  auto coords = cell_coords(point);
  if (!is_cell_valid(coords)) {
    return false;
  }
  for (auto id : cell(coords).entries) {
    const auto& e = entities_.find(id)->second;
    const auto& c = *e.collision;
    if (!(c.flags & mask) || !c.check_point) {
      continue;
    }
    auto min = e.transform->centre - e.collision->bounding_width;
    auto max = e.transform->centre + e.collision->bounding_width;
    if (intersect_aabb_point(min, max, point) && +c.check_point(e.handle, point, mask).mask) {
      return true;
    }
  }
  return false;
}

bool GridCollisionIndex::collide_line_any(const vec2& a, const vec2& b, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  auto c = cell_coords(a);
  auto end = cell_coords(b);
  ivec2 cv{end.x > c.x ? 1 : end.x == c.x ? 0 : -1, end.y > c.y ? 1 : end.y == c.y ? 0 : -1};
  while (true) {
    if (is_cell_valid(c)) {
      for (auto id : cell(c).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_line) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (intersect_aabb_line(b_min, b_max, a, b) && +c.check_line(e.handle, a, b, mask).mask) {
          return true;
        }
      }
    }
    if (c == end) {
      break;
    }
    if (c.x == end.x) {
      c += cv.y;
    } else if (c.y == end.y) {
      c += cv.x;
    } else if (intersect_aabb_line(cell_position({c.x, c.y + cv.y}),
                                   cell_position({c.x + 1, c.y + cv.y + 1}), a, b)) {
      c += ivec2{0, cv.y};
    } else {
      c += ivec2{cv.x, 0};
    }
  }
  return false;
}

bool GridCollisionIndex::collide_ball_any(const vec2& centre, fixed radius, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  auto min = min_coords(centre - radius);
  auto max = max_coords(centre + radius);
  for (std::int32_t y = min.y; y <= max.y; ++y) {
    for (std::int32_t x = min.x; x <= max.x; ++x) {
      for (auto id : cell({x, y}).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_ball) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (intersect_aabb_ball(b_min, b_max, centre, radius) &&
            +c.check_ball(e.handle, centre, radius, mask).mask) {
          return true;
        }
      }
    }
  }
  return false;
}

bool GridCollisionIndex::collide_convex_any(std::span<const vec2> convex, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  if (convex.empty()) {
    return false;
  }
  std::optional<ivec2> min;
  std::optional<ivec2> max;
  for (const auto& v : convex) {
    min = min ? glm::min(*min, min_coords(v)) : min_coords(v);
    max = max ? glm::max(*max, max_coords(v)) : max_coords(v);
  }
  for (std::int32_t y = min->y; y < max->y; ++y) {
    for (std::int32_t x = min->x; x < max->x; ++x) {
      for (auto id : cell({x, y}).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_convex) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (intersect_aabb_convex(b_min, b_max, convex) &&
            +c.check_convex(e.handle, convex, mask).mask) {
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collide_point(const vec2& point, shape_flag mask) const {
  std::vector<SimInterface::collision_info> r;
  auto coords = cell_coords(point);
  if (!is_cell_valid(coords)) {
    return r;
  }
  for (auto id : cell(coords).entries) {
    const auto& e = entities_.find(id)->second;
    const auto& c = *e.collision;
    if (!(c.flags & mask) || !c.check_point) {
      continue;
    }
    auto min = e.transform->centre - e.collision->bounding_width;
    auto max = e.transform->centre + e.collision->bounding_width;
    if (!intersect_aabb_point(min, max, point)) {
      continue;
    }
    if (auto hit = c.check_point(e.handle, point, mask); +hit.mask) {
      assert(r.empty() || e.handle.id() > r.back().h.id());
      r.emplace_back(SimInterface::collision_info{
          .h = e.handle, .hit_mask = hit.mask, .shape_centres = std::move(hit.shape_centres)});
    }
  }
  return r;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collide_line(const vec2& a, const vec2& b, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  std::vector<SimInterface::collision_info> r;
  auto c = cell_coords(a);
  auto end = cell_coords(b);
  ivec2 cv{end.x > c.x ? 1 : end.x == c.x ? 0 : -1, end.y > c.y ? 1 : end.y == c.y ? 0 : -1};
  while (true) {
    if (is_cell_valid(c)) {
      for (auto id : cell(c).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_line) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (!intersect_aabb_line(b_min, b_max, a, b)) {
          continue;
        }
        if (auto hit = c.check_line(e.handle, a, b, mask); +hit.mask) {
          r.emplace_back(SimInterface::collision_info{
              .h = e.handle, .hit_mask = hit.mask, .shape_centres = std::move(hit.shape_centres)});
        }
      }
    }
    if (c == end) {
      break;
    }
    if (c.x == end.x) {
      c += cv.y;
    } else if (c.y == end.y) {
      c += cv.x;
    } else if (intersect_aabb_line(cell_position({c.x, c.y + cv.y}),
                                   cell_position({c.x + 1, c.y + cv.y + 1}), a, b)) {
      c += ivec2{0, cv.y};
    } else {
      c += ivec2{cv.x, 0};
    }
  }
  std::sort(r.begin(), r.end(), [](const auto& a, const auto& b) { return a.h.id() < b.h.id(); });
  return r;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collide_ball(const vec2& centre, fixed radius, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  std::vector<SimInterface::collision_info> r;
  auto min = min_coords(centre - radius);
  auto max = max_coords(centre + radius);
  for (std::int32_t y = min.y; y <= max.y; ++y) {
    for (std::int32_t x = min.x; x <= max.x; ++x) {
      for (auto id : cell({x, y}).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_ball) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (!intersect_aabb_ball(b_min, b_max, centre, radius)) {
          continue;
        }
        if (auto hit = c.check_ball(e.handle, centre, radius, mask); +hit.mask) {
          r.emplace_back(SimInterface::collision_info{
              .h = e.handle, .hit_mask = hit.mask, .shape_centres = std::move(hit.shape_centres)});
        }
      }
    }
  }
  std::sort(r.begin(), r.end(), [](const auto& a, const auto& b) { return a.h.id() < b.h.id(); });
  return r;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collide_convex(std::span<const vec2> convex, shape_flag mask) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();
  std::vector<SimInterface::collision_info> r;
  if (convex.empty()) {
    return r;
  }
  std::optional<ivec2> min;
  std::optional<ivec2> max;
  for (const auto& v : convex) {
    min = min ? glm::min(*min, min_coords(v)) : min_coords(v);
    max = max ? glm::max(*max, max_coords(v)) : max_coords(v);
  }
  for (std::int32_t y = min->y; y < max->y; ++y) {
    for (std::int32_t x = min->x; x < max->x; ++x) {
      for (auto id : cell({x, y}).entries) {
        if (checked.contains(id)) {
          continue;
        }
        checked.emplace(id);
        const auto& e = entities_.find(id)->second;
        const auto& c = *e.collision;
        if (!(c.flags & mask) || !c.check_convex) {
          continue;
        }
        auto b_min = e.transform->centre - e.collision->bounding_width;
        auto b_max = e.transform->centre + e.collision->bounding_width;
        if (!intersect_aabb_convex(b_min, b_max, convex)) {
          continue;
        }
        if (auto hit = c.check_convex(e.handle, convex, mask); +hit.mask) {
          r.emplace_back(SimInterface::collision_info{
              .h = e.handle, .hit_mask = hit.mask, .shape_centres = std::move(hit.shape_centres)});
        }
      }
    }
  }
  std::sort(r.begin(), r.end(), [](const auto& a, const auto& b) { return a.h.id() < b.h.id(); });
  return r;
}

void GridCollisionIndex::in_range(const vec2& point, fixed distance, ecs::component_id cid,
                                  std::size_t max_n,
                                  std::vector<SimInterface::range_info>& output) const {
  auto min = min_coords(point - distance);
  auto max = max_coords(point + distance);
  auto output_begin = output.size();
  fixed max_distance = 0;
  std::size_t max_index = 0;
  // TODO: start from centre cell and work outwards, exiting if minimum possible distance is
  // too great (or output full).
  for (std::int32_t y = min.y; y <= max.y; ++y) {
    for (std::int32_t x = min.x; x <= max.x; ++x) {
      for (auto id : cell(ivec2{x, y}).centres) {
        const auto& e = entities_.find(id)->second;
        if (!e.handle.has(cid)) {
          continue;
        }
        auto d = e.transform->centre - point;
        auto d_sq = length_squared(d);
        if (d_sq <= distance * distance && (!max_n || output.size() - output_begin < max_n)) {
          if (!max_distance || d_sq > max_distance) {
            max_distance = d_sq;
            max_index = output.size();
          }
          output.emplace_back(SimInterface::range_info{e.handle, d, d_sq});
          continue;
        }
        if (d_sq > max_distance) {
          continue;
        }
        output[max_index] = {e.handle, d, d_sq};
        max_distance = 0;
        for (std::size_t i = output_begin; i < output.size(); ++i) {
          auto& o = output[i];
          if (!max_distance || o.distance_sq > max_distance) {
            max_distance = o.distance_sq;
            max_index = i;
          }
        }
      }
    }
  }
  std::sort(output.begin() + output_begin, output.end(),
            [](const auto& a, const auto& b) { return a.h.id() < b.h.id(); });
}

ivec2 GridCollisionIndex::cell_position(const ivec2& c) const {
  return {c.x << cell_power_.x, c.y << cell_power_.y};
}

ivec2 GridCollisionIndex::cell_coords(const ivec2& v) const {
  return {v.x >> cell_power_.x, v.y >> cell_power_.y};
}

ivec2 GridCollisionIndex::cell_coords(const vec2& v) const {
  return cell_coords(ivec2{v.x.to_int(), v.y.to_int()});
}

ivec2 GridCollisionIndex::min_coords(const vec2& v) const {
  return glm::max(cell_coords(v), cell_offset_);
}

ivec2 GridCollisionIndex::max_coords(const vec2& v) const {
  return glm::min(cell_coords(v), cell_offset_ + cell_count_ - ivec2{1, 1});
}

bool GridCollisionIndex::is_cell_valid(const ivec2& cell_coords) const {
  auto c = cell_coords - cell_offset_;
  return glm::all(glm::greaterThanEqual(c, ivec2{0, 0})) && glm::all(glm::lessThan(c, cell_count_));
}

auto GridCollisionIndex::cell(const ivec2& cell_coords) const -> const cell_t& {
  auto c = cell_coords - cell_offset_;
  assert(c.y * cell_count_.x + c.x < cells_.size());
  return cells_[c.y * cell_count_.x + c.x];
}

auto GridCollisionIndex::cell(const ivec2& cell_coords) -> cell_t& {
  auto c = cell_coords - cell_offset_;
  assert(c.y * cell_count_.x + c.x < cells_.size());
  return cells_[c.y * cell_count_.x + c.x];
}

void GridCollisionIndex::clear_cells(ecs::entity_id id, entry_t& e) {
  for (std::int32_t y = e.min.y; y <= e.max.y; ++y) {
    for (std::int32_t x = e.min.x; x <= e.max.x; ++x) {
      cell(ivec2{x, y}).clear(id);
    }
  }
  if (is_cell_valid(e.centre)) {
    cell(e.centre).clear_centre(id);
  }
}

void GridCollisionIndex::insert_cells(ecs::entity_id id, entry_t& e) {
  e.min = min_coords(e.transform->centre - e.collision->bounding_width);
  e.max = max_coords(e.transform->centre + e.collision->bounding_width);
  e.centre = cell_coords(e.transform->centre);
  for (std::int32_t y = e.min.y; y <= e.max.y; ++y) {
    for (std::int32_t x = e.min.x; x <= e.max.x; ++x) {
      cell(ivec2{x, y}).insert(id);
    }
  }
  if (is_cell_valid(e.centre)) {
    cell(e.centre).insert_centre(id);
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

void GridCollisionIndex::cell_t::insert_centre(ecs::entity_id id) {
  auto it = centres.begin();
  while (it != centres.end() && *it < id) {
    ++it;
  }
  centres.insert(it, id);
}

void GridCollisionIndex::cell_t::clear_centre(ecs::entity_id id) {
  centres.erase(std::find(centres.begin(), centres.end(), id));
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

void LegacyCollisionIndex::update(ecs::handle&) {}

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

bool LegacyCollisionIndex::collide_point_any(const vec2& point, shape_flag mask) const {
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
    if (+(e.flags & mask) && e.check_point && +e.check_point(collision.handle, point, mask).mask) {
      return true;
    }
  }
  return false;
}

bool LegacyCollisionIndex::collide_line_any(const vec2&, const vec2&, shape_flag) const {
  return false;  // Not used in legacy game.
}

bool LegacyCollisionIndex::collide_ball_any(const vec2&, fixed, shape_flag) const {
  return false;  // Not used in legacy game.
}

bool LegacyCollisionIndex::collide_convex_any(std::span<const vec2>, shape_flag) const {
  return false;  // Not used in legacy game.
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collide_point(const vec2& point, shape_flag mask) const {
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
    if (!(e.flags & mask) || !e.check_point) {
      continue;
    }
    if (auto hit = e.check_point(collision.handle, point, mask); +hit.mask) {
      r.emplace_back(SimInterface::collision_info{.h = collision.handle,
                                                  .hit_mask = hit.mask,
                                                  .shape_centres = std::move(hit.shape_centres)});
    }
  }
  return r;
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collide_line(const vec2&, const vec2&, shape_flag) const {
  return {};  // Not used in legacy game.
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collide_ball(const vec2&, fixed, shape_flag) const {
  return {};  // Not used in legacy game.
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collide_convex(std::span<const vec2>, shape_flag) const {
  return {};  // Not used in legacy game.
}

void LegacyCollisionIndex::in_range(const vec2&, fixed, ecs::component_id, std::size_t,
                                    std::vector<SimInterface::range_info>&) const {
  // Not used in legacy game (for now, at least).
}

}  // namespace ii
