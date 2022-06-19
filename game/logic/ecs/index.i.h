#ifndef II_GAME_LOGIC_ECS_INDEX_I_H
#define II_GAME_LOGIC_ECS_INDEX_I_H
#include "game/logic/ecs/index.h"
#include <algorithm>
#include <deque>

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
  virtual ~component_storage_base() = default;
  virtual void compact(EntityIndex& index) = 0;
  virtual void remove_index(handle h, std::size_t index) = 0;
};

template <Component C>
struct component_storage : component_storage_base {
  struct entry {
    entity_id id;
    std::optional<C> data;
  };
  std::deque<entry> entries;
  std::vector<EntityIndex::component_add_callback<C>> add_callbacks;
  std::vector<EntityIndex::component_remove_callback<C>> remove_callbacks;

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

  void remove_index(handle h, std::size_t index) override {
    for (auto& f : remove_callbacks) {
      f(h, *entries[index].data);
    }
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
void EntityIndex::on_component_add(const component_add_callback<C>& f) {
  storage<C>().add_callbacks.emplace_back(f);
}

template <Component C>
void EntityIndex::on_component_remove(const component_remove_callback<C>& f) {
  storage<C>().remove_callbacks.emplace_back(f);
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
    C& handle_base<Const>::emplace(Args&&... args) const requires(!Const) &&
    std::constructible_from<C, Args...> {
  auto& storage = index_->template storage<C>();
  auto index = table_->template get<C>();
  if (index) {
    auto& e = storage.entries[*index];
    e.data.emplace(std::forward<Args>(args)...);
    return *e.data;
  }
  index = static_cast<index_type>(storage.entries.size());
  auto& e = storage.entries.emplace_back();
  e.id = id();
  e.data.emplace(std::forward<Args>(args)...);
  table_->template set<C>(*index);
  for (const auto& f : storage.add_callbacks) {
    f(*this, *e.data);
  }
  return *e.data;
}

template <bool Const>
template <typename C>
C& handle_base<Const>::add(C&& data) const requires Component<std::remove_cvref_t<C>> {
  return emplace<std::remove_cvref_t<C>>(std::forward<C>(data));
}

template <bool Const>
template <Component C>
void handle_base<Const>::remove() const requires(!Const) {
  if (auto index = table_->template get<C>(); index) {
    auto& c = index_->template storage<C>();
    for (const auto& f : c.remove_callbacks()) {
      f(*this, *c.entries[*index].data);
    }
    table_->template reset<C>();
    c.entries[*index].data.reset();
  }
}

template <bool Const>
void handle_base<Const>::clear() const requires(!Const) {
  for (std::size_t i = 0; i < table_->v.size(); ++i) {
    if (table_->v[i]) {
      index_->components_[i]->remove_index(*this, *table_->v[i]);
    }
  }
  table_->v.clear();
}

template <bool Const>
template <Component C>
bool handle_base<Const>::has() const {
  return table_->template get<C>().has_value();
}

template <bool Const>
template <Component C>
C* handle_base<Const>::get() const requires(!Const) {
  if (auto index = table_->template get<C>(); index) {
    return &*index_->template storage<C>().entries[*index].data;
  }
  return nullptr;
}

template <bool Const>
template <Component C>
const C* handle_base<Const>::get() const requires Const {
  if (auto index = table_->template get<C>(); index) {
    return &*index_->template storage<C>()->entries[*index].data;
  }
  return nullptr;
}

}  // namespace ii::ecs

#endif