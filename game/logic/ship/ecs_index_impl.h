#ifndef II_GAME_LOGIC_SHIP_ECS_INDEX_IMPL_H
#define II_GAME_LOGIC_SHIP_ECS_INDEX_IMPL_H
#include "game/logic/ship/ecs_index.h"

namespace ii::ecs::detail {

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
  std::vector<std::optional<index_type>> v;
};

struct component_storage_base {
  virtual ~component_storage_base() {}
  virtual void compact(EntityIndex& index) = 0;
  virtual void reset_index(std::size_t index) = 0;
};

template <Component C>
struct component_storage : component_storage_base {
  struct entry {
    entity_id id;
    std::optional<C> data;
  };
  std::deque<entry> entries;

  void compact(EntityIndex& index) override {
    auto has_value = [](const entry& e) { return e.data.has_value(); };
    auto it = std::ranges::find_if_not(entries, has_value);
    if (it == entries.end()) {
      return;
    }
    auto [pivot, _] = std::ranges::stable_partition(it, entries.end(), has_value);
    for (index_type c_index = std::distance(entries.begin(), it); it != pivot; ++it) {
      (*index.entities_.find(it->id)->second)[ecs::id<C>()] = c_index++;
    }
    entries.erase(pivot, entries.end());
  }

  void reset_index(std::size_t index) override {
    entries[index].data.reset();
  }
};

}  // namespace ii::ecs::detail
namespace ii::ecs {

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
  auto [it, _] = entities_.emplace(id, std::make_unique<detail::component_table>());
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
  for (std::size_t i = 0; i < c.entries.size(); ++i) {
    auto& e = c.entries[i];
    if (e.data) {
      std::invoke(f, *e.data);
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<const C&> auto&& f) const {
  if (auto* c = storage<C>(); c) {
    for (std::size_t i = 0; i < c->entries.size(); ++i) {
      const auto& e = c->entries[i];
      if (e.data) {
        std::invoke(f, *e.data);
      }
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<handle, C&> auto&& f) {
  auto& c = storage<C>();
  for (std::size_t i = 0; i < c.entries.size(); ++i) {
    auto& e = c.entries[i];
    if (e.data) {
      std::invoke(f, *get(e.id), *e.data);
    }
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<const_handle, const C&> auto&& f) const {
  if (auto* c = storage<C>(); c) {
    for (std::size_t i = 0; i < c->entries.size(); ++i) {
      const auto& e = c->entries[i];
      if (e.data) {
        std::invoke(f, *get(e.id), *e.data);
      }
    }
  }
}

template <Component C>
auto EntityIndex::storage() -> detail::component_storage<C>& {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  if (c_id >= components_.size()) {
    components_.resize(1 + c_id);
  }
  if (!components_[c_id]) {
    components_[c_id] = std::make_unique<detail::component_storage<C>>();
  }
  return static_cast<detail::component_storage<C>&>(*components_[c_id]);
}

template <Component C>
auto EntityIndex::storage() const -> const detail::component_storage<C>* {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < components_.size() && components_[c_id]
      ? static_cast<detail::component_storage<C>*>(components_[c_id].get())
      : nullptr;
}

template <bool Const>
template <Component C, typename... Args>
C& handle_base<Const>::emplace(Args&&... args) const requires !Const {
  auto& storage = index_->storage<C>();
  auto index = table_->get<C>();
  if (!index) {
    index = static_cast<index_type>(storage.entries.size());
    storage.entries.emplace_back().id = id();
    table_->set<C>(*index);
  }
  auto& e = storage.entries[*index];
  e.data.emplace(std::forward<Args>(args)...);
  return *e.data;
}

template <bool Const>
template <Component C>
void handle_base<Const>::remove() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    table_->reset<C>();
    index_->storage<C>().entries[*index].data.reset();
  }
}

template <bool Const>
void handle_base<Const>::clear() const requires !Const {
  for (std::size_t i = 0; i < table_->v.size(); ++i) {
    if (table_->v[i]) {
      index_->components_[i]->reset_index(*table_->v[i]);
    }
  }
  table_->v.clear();
}

template <bool Const>
template <Component C>
bool handle_base<Const>::has() const {
  return table_->get<C>().has_value();
}

template <bool Const>
template <Component C>
C* handle_base<Const>::get() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>().entries[*index].data;
  }
  return nullptr;
}

template <bool Const>
template <Component C>
const C* handle_base<Const>::get() const requires Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>()->entries[*index].data;
  }
  return nullptr;
}

}  // namespace ii::ecs

#endif