
#ifndef II_GAME_LOGIC_SHIP_ECS_H
#define II_GAME_LOGIC_SHIP_ECS_H
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

class EntityIndex {
private:
  struct component_table;

  template <bool Const>
  class handle_base {
  public:
    entity_id id() const {
      return id_;
    }

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
    using table_t = std::conditional_t<Const, const component_table, component_table>;

    handle_base(entity_id e_id, index_t* index, table_t* table)
    : id_{e_id}, index_{index}, table_{table} {}

    entity_id id_;
    index_t* index_;
    table_t* table_;
  };

public:
  using handle = handle_base<false>;
  using const_handle = handle_base<true>;

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
  struct component_table;
  struct component_storage_base {
    virtual ~component_storage_base() {}
    virtual void compact(EntityIndex& index) = 0;
    virtual void reset_index(std::size_t index) = 0;
  };

  template <Component C>
  struct component_storage;
  template <Component C>
  component_storage<C>& storage();
  template <Component C>
  const component_storage<C>* storage() const;

  entity_id next_id_{0};
  std::unordered_map<entity_id, std::unique_ptr<component_table>> entities_;
  std::vector<std::unique_ptr<component_storage_base>> components_;
};

struct EntityIndex::component_table {
  template <Component C>
  void set(index_type index) {
    auto c_index = static_cast<std::size_t>(ecs::id<C>());
    if (c_index >= v.size()) {
      v.resize(c_index + 1);
    }
    v[c_index] = index;
  }
  template <Component C>
  void clear() {
    auto c_index = static_cast<std::size_t>(ecs::id<C>());
    v[c_index].reset();
  }
  template <Component C>
  std::optional<index_type> get() const {
    auto c_index = static_cast<std::size_t>(ecs::id<C>());
    return c_index < v.size() ? v[c_index] : std::optional<index_type>{};
  }
  std::vector<std::optional<index_type>> v;
};

template <Component C>
struct EntityIndex::component_storage : EntityIndex::component_storage_base {
  struct entry {
    entity_id id;
    std::optional<C> data;
  };
  std::deque<entry> entries;

  void compact(EntityIndex& index) override;
  void reset_index(std::size_t index) override {
    entries[index].data.reset();
  }
};

inline void EntityIndex::compact() {
  for (const auto& c : components_) {
    if (c) {
      c->compact(*this);
    }
  }
}

inline auto EntityIndex::create() -> handle {
  // TODO: is ID reuse OK?
  auto id = next_id_++;
  while (contains(id)) {
    id = next_id_++;
  }
  auto [it, _] = entities_.emplace(id, std::make_unique<component_table>());
  return {id, this, it->second.get()};
}

template <Component... CArgs>
auto EntityIndex::create(CArgs&&... cargs) -> handle {
  auto h = create();
  (h.emplace<std::remove_cvref_t<CArgs>>(std::forward<CArgs>(cargs)), ...);
  return h;
}

inline void EntityIndex::destroy(entity_id id) {
  auto it = entities_.find(id);
  if (it != entities_.end()) {
    handle{id, this, it->second.get()}.clear();
    entities_.erase(it);
  }
}

inline auto EntityIndex::get(entity_id id) -> std::optional<handle> {
  std::optional<handle> r;
  if (auto it = entities_.find(id); it != entities_.end()) {
    r = {id, this, it->second.get()};
  }
  return r;
}

inline auto EntityIndex::get(entity_id id) const -> std::optional<const_handle> {
  std::optional<const_handle> r;
  if (auto it = entities_.find(id); it != entities_.end()) {
    r = {id, this, it->second.get()};
  }
  return r;
}

template <Component C>
bool EntityIndex::has(entity_id id) const {
  auto h = get(id);
  return h && h->has<C>();
}

template <Component C>
C* EntityIndex::get(entity_id id) {
  auto h = get(id);
  return h ? h->get<C>() : nullptr;
}

template <Component C>
const C* EntityIndex::get(entity_id id) const {
  auto h = get(id);
  return h ? h->get<C>() : nullptr;
}

template <Component C>
void EntityIndex::iterate(std::invocable<C&> auto&& f) {
  auto& c = storage<C>();
  for (auto& e : c.entries) {
    if (e.data) {
      std::invoke(f, *e.data);
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<const C&> auto&& f) const {
  if (auto* c = storage<C>(); c) {
    for (auto& e : c->entries) {
      if (e.data) {
        std::invoke(f, *e.data);
      }
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<handle, C&> auto&& f) {
  auto& c = storage<C>();
  for (auto& e : c.entries) {
    if (e.data) {
      std::invoke(f, *get(e.id), *e.data);
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<const_handle, const C&> auto&& f) const {
  if (auto* c = storage<C>(); c) {
    for (auto& e : c->entries) {
      if (e.data) {
        std::invoke(f, *get(e.id), *e.data);
      }
    }
  }
}

template <Component C>
auto EntityIndex::storage() -> component_storage<C>& {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  if (c_id >= components_.size()) {
    components_.resize(1 + c_id);
  }
  if (!components_[c_id]) {
    components_[c_id] = std::make_unique<component_storage<C>>();
  }
  return static_cast<component_storage<C>&>(*components_[c_id]);
}

template <Component C>
auto EntityIndex::storage() const -> const component_storage<C>* {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < components_.size() && components_[c_id]
      ? static_cast<component_storage<C>*>(components_[c_id].get())
      : nullptr;
}

template <bool Const>
template <Component C, typename... Args>
C& EntityIndex::handle_base<Const>::emplace(Args&&... args) const requires !Const {
  auto& storage = index_->storage<C>();
  auto index = table_->get<C>();
  if (!index) {
    index = static_cast<index_type>(storage.entries.size());
    storage.entries.emplace_back().id = id();
  }
  auto& e = storage.entries[*index];
  e.data.emplace(std::forward<Args>(args)...);
  return *e.data;
}

template <bool Const>
template <Component C>
void EntityIndex::handle_base<Const>::remove() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    table_->clear<C>();
    index_->storage<C>().entres[*index].data.reset();
  }
}

template <bool Const>
void EntityIndex::handle_base<Const>::clear() const requires !Const {
  for (std::size_t i = 0; i < table_->v.size(); ++i) {
    if (table_->v[i]) {
      index_->components_[i]->reset_index(*table_->v[i]);
    }
  }
  table_->v.clear();
}

template <bool Const>
template <Component C>
bool EntityIndex::handle_base<Const>::has() const {
  return table_->get<C>().has_value();
}

template <bool Const>
template <Component C>
C* EntityIndex::handle_base<Const>::get() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>().entries[*index].data;
  }
  return nullptr;
}

template <bool Const>
template <Component C>
const C* EntityIndex::handle_base<Const>::get() const requires Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>()->entries[*index].data;
  }
  return nullptr;
}

template <Component C>
void EntityIndex::component_storage<C>::compact(EntityIndex& index) {
  auto has_value = [](const entry& e) { return e.data.has_value(); };
  auto it = std::ranges::find_if_not(entries, has_value);
  if (it == entries.end()) {
    return;
  }
  auto [pivot, _] = std::ranges::stable_partition(it, entries.end(), has_value);
  for (index_type index = std::distance(entries.begin(), it); it != pivot; ++it, ++index) {
    index.entities_.find(it->id)->second->v[ecs::id<C>()] = index;
  }
  entries.erase(pivot, entries.end());
}

}  // namespace ii::ecs

#endif