#ifndef II_GAME_LOGIC_ECS_DETAIL_H
#define II_GAME_LOGIC_ECS_DETAIL_H
#include "game/logic/ecs/id.h"
#include <optional>
#include <vector>

namespace ii::ecs::detail {
template <Component>
struct component_storage;
struct component_storage_base;

struct component_table {
  std::optional<index_type>& operator[](component_id index) {
    return v[static_cast<std::size_t>(index)];
  }
  const std::optional<index_type>& operator[](component_id index) const {
    return v[static_cast<std::size_t>(index)];
  }

  template <Component C>
  void set(index_type index) {
    auto c_index = static_cast<std::size_t>(ecs::id<C>());
    if (c_index >= v.size()) {
      v.resize(c_index + 1);
    }
    v[c_index] = index;
  }
  template <Component C>
  void reset() {
    (*this)[ecs::id<C>()].reset();
  }
  template <Component C>
  std::optional<index_type> get() const {
    auto c_index = static_cast<std::size_t>(ecs::id<C>());
    return c_index < v.size() ? v[c_index] : std::optional<index_type>{};
  }
  std::optional<entity_id> id;
  std::vector<std::optional<index_type>> v;
};

}  // namespace ii::ecs::detail

#endif