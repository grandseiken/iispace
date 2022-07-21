#ifndef II_GAME_LOGIC_ECS_INDEX_H
#define II_GAME_LOGIC_ECS_INDEX_H
#include "game/logic/ecs/id.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ii::ecs {
class EntityIndex;

namespace detail {
struct component_storage_base;
struct component_table;
template <Component C>
struct component_storage;
}  // namespace detail

template <bool Const>
class handle_base {
public:
  handle_base(handle_base&&) noexcept = default;
  handle_base(const handle_base&) = default;
  handle_base& operator=(handle_base&&) noexcept = default;
  handle_base& operator=(const handle_base&) = default;
  handle_base(const handle_base<false>& h) requires Const
  : handle_base{h.id_, h.index_, h.table_} {}

  entity_id id() const {
    return id_;
  }

  // Add a component via in-place construction.
  template <Component C, typename... Args>
      C& emplace(Args&&... args) const requires(!Const) && std::constructible_from<C, Args...>;
  // Add a component.
  template <typename C>
  C& add(C&& data) const requires Component<std::remove_cvref_t<C>>;
  // Remove a component. Invalidates any direct data reference to the component for this element.
  template <Component C>
  void remove() const requires(!Const);
  // Clear all components. Invalidates all direct data references to any component for this
  // element.
  void clear() const requires(!Const);

  // Check existence of a component.
  template <Component C>
  bool has() const;
  // Obtain pointer to component data, if it exists.
  template <Component C>
  C* get() const requires(!Const);
  // Obtain pointer to component data, if it exists.
  template <Component C>
  const C* get() const requires Const;

private:
  friend class handle_base<!Const>;
  friend class EntityIndex;
  using index_t = std::conditional_t<Const, const EntityIndex, EntityIndex>;
  using table_t = std::conditional_t<Const, const detail::component_table, detail::component_table>;

  handle_base(entity_id e_id, index_t* index, table_t* table)
  : id_{e_id}, index_{index}, table_{table} {}

  entity_id id_{0};
  index_t* index_ = nullptr;
  table_t* table_ = nullptr;
};

using handle = handle_base<false>;
using const_handle = handle_base<true>;

class EntityIndex {
public:
  EntityIndex() = default;
  // Moving the index invalidates all handles.
  EntityIndex(EntityIndex&&) = default;
  EntityIndex(const EntityIndex&) = delete;
  EntityIndex& operator=(EntityIndex&&) = default;
  EntityIndex& operator=(const EntityIndex&) = delete;

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

  std::size_t size() const {
    return entities_.size();
  }
  // Check existence of an element.
  bool contains(entity_id id) const {
    return entities_.contains(id);
  }
  // Return handle for an element.
  std::optional<handle> get(entity_id id);
  std::optional<const_handle> get(entity_id id) const;

  // Count number of entities with component.
  template <Component C>
  index_type count() const;
  // Check existence of component on an element.
  template <Component C>
  bool has(entity_id id) const;
  // Obtain pointer to component data for an element, if it exists.
  template <Component C>
  C* get(entity_id id);
  // Obtain pointer to component data for an element, if it exists.
  template <Component C>
  const C* get(entity_id id) const;

  template <typename C>
  using component_add_callback = std::function<void(handle, C&)>;
  template <typename C>
  using component_remove_callback = std::function<void(handle, const C&)>;

  // Add a handler to be called whenever the component is added.
  template <Component C>
  void on_component_add(const component_add_callback<C>& f);
  // Add a handler to be called whenever the component is removed.
  template <Component C>
  void on_component_remove(const component_remove_callback<C>& f);

  template <Component C>
  void iterate(std::invocable<C&> auto&& f, bool include_new = true);
  template <Component C>
  void iterate(std::invocable<const C&> auto&& f, bool include_new = true) const;
  template <Component C>
  void iterate_dispatch(auto&& f, bool include_new = true);
  template <Component C>
  void iterate_dispatch(auto&& f, bool include_new = true) const;
  template <Component C>
  void iterate_dispatch_if(auto&& f, bool include_new = true);
  template <Component C>
  void iterate_dispatch_if(auto&& f, bool include_new = true) const;

private:
  template <bool>
  friend class handle_base;
  template <Component>
  friend struct detail::component_storage;

  template <Component C>
  detail::component_storage<C>& storage();
  template <Component C>
  const detail::component_storage<C>* storage() const;

  entity_id next_id_{0};
  std::unordered_map<entity_id, std::unique_ptr<detail::component_table>> entities_;
  std::vector<std::unique_ptr<detail::component_storage_base>> components_;
  std::vector<std::unique_ptr<detail::component_table>> table_pool_;
};

}  // namespace ii::ecs

#include "game/logic/ecs/index.i.h"
#endif
