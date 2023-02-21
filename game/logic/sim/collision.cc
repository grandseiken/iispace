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

void GridCollisionIndex::refresh_handles(const SimInterface& interface, ecs::EntityIndex& index) {
  interface_ = &interface;
  for (auto& pair : entities_) {
    auto& e = pair.second;
    e.handle = *index.get(e.id);
    e.collision = e.handle.get<Collision>();
    e.transform = e.handle.get<Transform>();
  }
}

void GridCollisionIndex::add(ecs::handle& h, const Collision& c) {
  if (c.check_collision) {
    auto [it, _] = entities_.emplace(h.id(), entry_t{h.id(), h, h.get<Transform>(), &c});
    insert_cells(h.id(), it->second);
  }
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

template <typename F>
void GridCollisionIndex::iterate_collision_cells(const geom2::check_t& it, const F& f) const {
  static thread_local std::unordered_set<ecs::entity_id> checked;
  checked.clear();

  auto handle_id = [&](ecs::entity_id id, auto&& check_bounds) {
    const auto& e = entities_.find(id)->second;
    const auto& c = *e.collision;
    if (!(c.flags & it.mask)) {
      return false;
    }
    auto min = e.transform->centre - e.collision->bounding_width;
    auto max = e.transform->centre + e.collision->bounding_width;
    if (!check_bounds(min, max)) {
      return false;
    }
    if (auto hit = c.check_collision(e.handle, it, *interface_); +hit.mask) {
      return f(e.handle, hit);
    }
    return false;
  };

  if (const auto* ic = std::get_if<geom2::check_point_t>(&it.extent)) {
    auto coords = cell_coords(ic->v);
    if (!is_cell_valid(coords)) {
      return;
    }
    for (auto id : cell(coords).entries) {
      if (handle_id(id, [&](const vec2& min, const vec2& max) {
            return intersect_aabb_point(min, max, ic->v);
          })) {
        break;
      }
    }
  } else if (const auto* ic = std::get_if<geom2::check_line_t>(&it.extent)) {
    auto c = cell_coords(ic->a);
    auto end = cell_coords(ic->b);
    ivec2 cv{end.x > c.x ? 1 : end.x == c.x ? 0 : -1, end.y > c.y ? 1 : end.y == c.y ? 0 : -1};
    bool done = false;
    while (!done) {
      if (is_cell_valid(c)) {
        for (auto id : cell(c).entries) {
          if (checked.contains(id)) {
            continue;
          }
          checked.emplace(id);
          if (handle_id(id, [&](const vec2& min, const vec2& max) {
                return intersect_aabb_line(min, max, ic->a, ic->b);
              })) {
            done = true;
            break;
          }
        }
      }
      if (done || c == end) {
        break;
      }
      if (c.x == end.x) {
        c.y += cv.y;
      } else if (c.y == end.y) {
        c.x += cv.x;
      } else if (intersect_aabb_line(cell_position({c.x, c.y + cv.y}),
                                     cell_position({c.x + 1, c.y + cv.y + 1}), ic->a, ic->b)) {
        c.y += cv.y;
      } else {
        c.x += cv.x;
      }
    }
  } else if (const auto* ic = std::get_if<geom2::check_ball_t>(&it.extent)) {
    auto min = min_coords(ic->c - ic->r);
    auto max = max_coords(ic->c + ic->r);
    bool done = false;
    for (std::int32_t y = min.y; !done && y <= max.y; ++y) {
      for (std::int32_t x = min.x; !done && x <= max.x; ++x) {
        for (auto id : cell({x, y}).entries) {
          if (checked.contains(id)) {
            continue;
          }
          checked.emplace(id);
          if (handle_id(id, [&](const vec2& b_min, const vec2& b_max) {
                return intersect_aabb_ball(b_min, b_max, ic->c, ic->r);
              })) {
            done = true;
            break;
          }
        }
      }
    }
  } else if (const auto* ic = std::get_if<geom2::check_convex_t>(&it.extent)) {
    if (ic->vs.empty()) {
      return;
    }
    std::optional<ivec2> min;
    std::optional<ivec2> max;
    for (const auto& v : ic->vs) {
      min = min ? glm::min(*min, min_coords(v)) : min_coords(v);
      max = max ? glm::max(*max, max_coords(v)) : max_coords(v);
    }
    bool done = false;
    for (std::int32_t y = min->y; !done && y <= max->y; ++y) {
      for (std::int32_t x = min->x; !done && x <= max->x; ++x) {
        for (auto id : cell({x, y}).entries) {
          if (checked.contains(id)) {
            continue;
          }
          checked.emplace(id);
          if (handle_id(id, [&](const vec2& b_min, const vec2& b_max) {
                return intersect_aabb_convex(b_min, b_max, ic->vs);
              })) {
            done = true;
            break;
          }
        }
      }
    }
  }
}

bool GridCollisionIndex::collide_any(const geom2::check_t& it) const {
  bool result = false;
  iterate_collision_cells(it, [&](ecs::handle, const geom2::hit_result&) { return result = true; });
  return result;
}

std::vector<SimInterface::collision_info>
GridCollisionIndex::collide(const geom2::check_t& it) const {
  std::vector<SimInterface::collision_info> r;
  iterate_collision_cells(it, [&](ecs::handle h, geom2::hit_result& hit) {
    r.emplace_back(SimInterface::collision_info{
        .h = h, .hit_mask = hit.mask, .shape_centres = std::move(hit.shape_centres)});
    return false;
  });
  if (!std::holds_alternative<geom2::check_point_t>(it.extent)) {
    std::sort(r.begin(), r.end(), [](const auto& a, const auto& b) { return a.h.id() < b.h.id(); });
  }
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

void LegacyCollisionIndex::refresh_handles(const SimInterface& interface, ecs::EntityIndex& index) {
  interface_ = &interface;
  for (auto& e : entries_) {
    e.handle = *index.get(e.id);
    e.collision = e.handle.get<Collision>();
    e.transform = e.handle.get<Transform>();
  }
}

void LegacyCollisionIndex::add(ecs::handle& h, const Collision& c) {
  if (c.check_collision) {
    entries_.emplace_back(entry{h.id(), h, h.get<Transform>(), &c, 0});
  }
}

void LegacyCollisionIndex::update(ecs::handle&) {}

void LegacyCollisionIndex::remove(ecs::handle& h) {
  if (auto it = std::find_if(entries_.begin(), entries_.end(),
                             [&](const auto& e) { return e.handle.id() == h.id(); });
      it != entries_.end()) {
    entries_.erase(it);
  }
}

void LegacyCollisionIndex::begin_tick() {
  for (auto& e : entries_) {
    e.x_min = e.transform->centre.x - e.collision->bounding_width;
  }
  std::stable_sort(entries_.begin(), entries_.end(),
                   [](const auto& a, const auto& b) { return a.x_min < b.x_min; });
}

bool LegacyCollisionIndex::collide_any(const geom2::check_t& it) const {
  const auto* c = std::get_if<geom2::check_point_t>(&it.extent);
  if (!c) {
    return false;
  }
  fixed x = c->v.x;
  fixed y = c->v.y;

  for (const auto& collision : entries_) {
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
    if (+(e.flags & it.mask) && +e.check_collision(collision.handle, it, *interface_).mask) {
      return true;
    }
  }
  return false;
}

std::vector<SimInterface::collision_info>
LegacyCollisionIndex::collide(const geom2::check_t& it) const {
  std::vector<SimInterface::collision_info> r;
  const auto* c = std::get_if<geom2::check_point_t>(&it.extent);
  if (!c) {
    return r;
  }
  fixed x = c->v.x;
  fixed y = c->v.y;

  for (const auto& collision : entries_) {
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
    if (!(e.flags & it.mask)) {
      continue;
    }
    if (auto hit = e.check_collision(collision.handle, it, *interface_); +hit.mask) {
      r.emplace_back(SimInterface::collision_info{.h = collision.handle,
                                                  .hit_mask = hit.mask,
                                                  .shape_centres = std::move(hit.shape_centres)});
    }
  }
  return r;
}

void LegacyCollisionIndex::in_range(const vec2&, fixed, ecs::component_id, std::size_t,
                                    std::vector<SimInterface::range_info>&) const {
  // Not used in legacy game (for now, at least).
}

}  // namespace ii
