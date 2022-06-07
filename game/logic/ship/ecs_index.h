#ifndef II_GAME_LOGIC_SHIP_ECS_INDEX_H
#define II_GAME_LOGIC_SHIP_ECS_INDEX_H
#include "game/common/enum.h"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ii::ecs {
template <typename Id, typename Tag = Id>
class sequential_id {
private:
  inline static Id identifier_{0};

public:
  template <typename...>
  inline static const Id value = identifier_++;
};

using index_type = std::uint32_t;
enum class entity_id : index_type {};
enum class component_id : index_type {};
}  // namespace ii::ecs

namespace ii {
template <>
struct integral_enum<ecs::entity_id> : std::true_type {};
template <>
struct integral_enum<ecs::component_id> : std::true_type {};
}  // namespace ii

namespace ii::ecs {

template <typename T>
concept Component = /* TODO */ true;

template <Component C>
component_id id() {
  return sequential_id<component_id>::value<C>;
}

namespace detail {
struct component_storage_base;
struct component_table;
template <Component C>
struct component_storage;
}  // namespace detail

template <bool Const>
class handle_base {
public:
  entity_id id() const {
    return id_;
  }

  handle_base(const handle_base&) = default;
  handle_base& operator=(const handle_base&) = default;
  handle_base(const handle_base<false>&) requires Const : handle_base{id_, index_, table_} {}

  // Add a component.
  template <Component C, typename... Args>
  C& emplace(Args&&... args) const requires !Const;
  // Remove a component. Invalidates any direct data reference to the component for this element.
  template <Component C>
  void remove() const requires !Const;
  // Clear all components. Invalidates all direct data references to any component for this
  // element.
  void clear() const requires !Const;

  // Check existence of a component.
  template <Component C>
  bool has() const;
  // Obtain pointer to component data, if it exists.
  template <Component C>
  C* get() const requires !Const;
  // Obtain pointer to component data, if it exists.
  template <Component C>
  const C* get() const requires Const;

private:
  friend class EntityIndex;
  using index_t = std::conditional_t<Const, const EntityIndex, EntityIndex>;
  using table_t = std::conditional_t<Const, const detail::component_table, detail::component_table>;

  handle_base(entity_id e_id, index_t* index, table_t* table)
  : id_{e_id}, index_{index}, table_{table} {}

  entity_id id_;
  index_t* index_;
  table_t* table_;
};

using handle = handle_base<false>;
using const_handle = handle_base<true>;

class EntityIndex {
public:
  EntityIndex() = default;
  // Moving the index invalidates all handles.
  EntityIndex(EntityIndex&&) = default;
  EntityIndex& operator=(EntityIndex&&) = default;

  // Rearrange and compact internals. Handles remain valid, but invalidates all direct data
  // references to all components.
  void compact();
  // Create a new element and return handle.
  handle create();
  // Create a new element and add each component provided.
  template <Component... CArgs>
  handle create(CArgs&&... cargs);
  // Remove an element along with all associated components. Invalidates handles for the element and
  // all direct data references to any of its components.
  void destroy(entity_id id);

  // Check existence of an element.
  bool contains(entity_id id) const {
    return entities_.contains(id);
  }
  // Return handle for an element.
  std::optional<handle> get(entity_id id);
  std::optional<const_handle> get(entity_id id) const;

  // Check existence of component on an element.
  template <Component C>
  bool has(entity_id id) const;
  // Obtain pointer to component data for an element, if it exists.
  template <Component C>
  C* get(entity_id id);
  // Obtain pointer to component data for an element, if it exists.
  template <Component C>
  const C* get(entity_id id) const;

  template <Component C>
  void iterate(std::invocable<C&> auto&& f);
  template <Component C>
  void iterate(std::invocable<const C&> auto&& f) const;
  template <Component C>
  void iterate(std::invocable<handle, C&> auto&& f);
  template <Component C>
  void iterate(std::invocable<const_handle, const C&> auto&& f) const;

private:
  template <bool>
  friend class handle_base;
  template <Component>
  friend class detail::component_storage;

  template <Component C>
  detail::component_storage<C>& storage();
  template <Component C>
  const detail::component_storage<C>* storage() const;

  entity_id next_id_{0};
  std::unordered_map<entity_id, std::unique_ptr<detail::component_table>> entities_;
  std::vector<std::unique_ptr<detail::component_storage_base>> components_;
};

}  // namespace ii::ecs

#include "game/logic/ship/ecs_index_impl.h"
#endif