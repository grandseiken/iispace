#ifndef II_GAME_LOGIC_ECS_INDEX_I_H
#define II_GAME_LOGIC_ECS_INDEX_I_H
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include <algorithm>
#include <deque>
#include <map>
#include <typeinfo>
#include <utility>

namespace ii::ecs::detail {
struct component_storage_base {
  virtual ~component_storage_base() = default;
  virtual void compact(EntityIndex& index) = 0;
  virtual void remove_index(handle h, std::size_t index) = 0;
  virtual void copy_clear() = 0;
  virtual void
  copy_to(EntityIndex& index, std::unique_ptr<component_storage_base>& target) const = 0;
  virtual void dump(std::size_t index, bool portable, Printer& printer) const = 0;
  virtual std::string debug_name() const = 0;
};

template <Component C>
struct component_storage_get : component_storage_base {
  struct entry {
    entity_id id;
    std::optional<C> data;
  };
  index_type size = 0;
  std::deque<entry> entries;
};

template <Component C>
struct component_storage : component_storage_get<C> {
  using base = component_storage_get<C>;
  using entry = typename base::entry;
  std::vector<EntityIndex::component_add_callback<C>> add_callbacks;
  std::vector<EntityIndex::component_remove_callback<C>> remove_callbacks;

  void compact(EntityIndex& index) override {
    auto has_value = [](const entry& e) { return e.data.has_value(); };
    auto it = std::find_if_not(base::entries.begin(), base::entries.end(), has_value);
    if (it == base::entries.end()) {
      return;
    }
    auto pivot = std::stable_partition(it, base::entries.end(), has_value);
    for (index_type c_index = std::distance(base::entries.begin(), it); it != pivot; ++it) {
      (*index.entities_.find(it->id)->second)[ecs::id<C>()] = c_index++;
    }
    base::entries.erase(pivot, base::entries.end());
  }

  void remove_index(handle h, std::size_t index) override {
    for (auto& f : remove_callbacks) {
      f(h, *base::entries[index].data);
    }
    base::entries[index].data.reset();
    --base::size;
  }

  void copy_clear() override {
    base::size = 0;
    base::entries.clear();
  }

  void
  copy_to(EntityIndex& index, std::unique_ptr<component_storage_base>& target_ptr) const override {
    if (!target_ptr) {
      target_ptr = std::make_unique<component_storage>();
    }
    auto& target = static_cast<component_storage&>(*target_ptr);
    target.size = base::size;
    target.entries.resize(base::size);
    for (index_type i = 0; const auto& e : base::entries) {
      if (e.data) {
        auto& e_copy = target.entries[i];
        e_copy.id = e.id;
        e_copy.data = *e.data;
        (*index.entities_[e.id])[ecs::id<C>()] = i++;
      }
    }
  }

  void dump(std::size_t index, bool portable, Printer& printer) const override {
    const auto& data = *base::entries[index].data;
    printer.begin().put('[');
    if (!portable) {
      printer.put(+ecs::id<C>()).put(" / ");
    }
    if constexpr (requires { to_debug_name(&data); }) {
      printer.put(to_debug_name(&data));
    } else {
      printer.put(typeid(C).name());
    }
    printer.put(']').end();
    if constexpr (requires { to_debug_tuple(data); }) {
      printer.indent();
      std::apply(
          [&](const auto&... members) {
            (printer.begin().put(members.name).put(": ").put(members.value).end(), ...);
          },
          to_debug_tuple(data));
      printer.undent();
    }
  }

  std::string debug_name() const override {
    C* p = nullptr;
    if constexpr (requires { to_debug_name(p); }) {
      return to_debug_name(p);
    } else {
      return {};
    }
  }
};

template <typename T>
void iterate(T& storage, bool include_new, auto&& f) {
  if (include_new) {
    for (std::size_t i = 0; i < storage.entries.size(); ++i) {
      if (storage.entries[i].data) {
        f(storage.entries[i]);
      }
    }
  } else {
    for (std::size_t i = 0, end = storage.entries.size(); i < end; ++i) {
      if (storage.entries[i].data) {
        f(storage.entries[i]);
      }
    }
  }
}
}  // namespace ii::ecs::detail

namespace ii::ecs {

inline void EntityIndex::dump(Printer& printer, bool portable, const query& q) const {
  std::map<std::string, std::pair<std::size_t, detail::component_storage_base*>> sorted_components;
  if (portable) {
    for (std::size_t i = 0; i < components_.size(); ++i) {
      if (components_[i]) {
        sorted_components.emplace(components_[i]->debug_name(),
                                  std::make_pair(i, components_[i].get()));
      }
    }
  }

  for (const auto& e : entity_tables_) {
    if (!e.id || (!q.entity_ids.empty() && !q.entity_ids.contains(*e.id))) {
      continue;
    }
    bool matches_components = q.components.empty();
    if (!matches_components) {
      for (std::size_t i = 0; i < e.v.size(); ++i) {
        if (e.v[i] && q.components.contains(components_[i]->debug_name())) {
          matches_components = true;
          break;
        }
      }
    }
    if (!matches_components) {
      continue;
    }
    printer.begin().put("[@ ").put(+*e.id).put("]").end().indent();
    if (portable) {
      for (const auto& pair : sorted_components) {
        auto i = pair.second.first;
        const auto* c = pair.second.second;
        if (i < e.v.size() && e.v[i] &&
            (q.components.empty() || q.components.contains(c->debug_name()))) {
          c->dump(*e.v[i], portable, printer);
        }
      }
    } else {
      for (std::size_t i = 0; i < e.v.size(); ++i) {
        if (e.v[i] &&
            (q.components.empty() || q.components.contains(components_[i]->debug_name()))) {
          components_[i]->dump(*e.v[i], portable, printer);
        }
      }
    }
    printer.undent();
    printer.end();
  }
}

inline void EntityIndex::copy_to(EntityIndex& target) const {
  target.next_id_ = next_id_;
  target.entities_.clear();
  auto size = entities_.size();
  if (target.entity_tables_.size() < size) {
    target.entity_tables_.resize(size);
  }
  for (std::size_t i = 0; const auto& table : entity_tables_) {
    if (table.id) {
      auto& table_copy = target.entity_tables_[i++];
      table_copy.id = *table.id;
      table_copy.v.clear();
      table_copy.v.resize(table.v.size());
      target.entities_.emplace(*table.id, &table_copy);
    }
  }
  for (std::size_t i = size; i < target.next_entity_table_index_; ++i) {
    target.entity_tables_[i].id.reset();
    target.entity_tables_[i].v.clear();
  }
  target.next_entity_table_index_ = size;
  target.components_.resize(components_.size());
  for (std::size_t i = 0; i < components_.size(); ++i) {
    if (components_[i]) {
      components_[i]->copy_to(target, target.components_[i]);
    } else if (target.components_[i]) {
      target.components_[i]->copy_clear();
    }
  }
}

inline void EntityIndex::compact() {
  auto has_value = [](const detail::component_table& table) { return table.id.has_value(); };
  auto end = entity_tables_.begin() + static_cast<std::ptrdiff_t>(next_entity_table_index_);
  auto it = std::find_if_not(entity_tables_.begin(), end, has_value);
  if (it == end) {
    return;
  }
  auto pivot = std::stable_partition(it, end, has_value);
  for (; it != pivot; ++it) {
    entities_[*it->id] = &*it;
  }
  next_entity_table_index_ = static_cast<std::size_t>(std::distance(entity_tables_.begin(), pivot));
  for (const auto& c : components_) {
    if (c) {
      c->compact(*this);
    }
  }
}

inline auto EntityIndex::create() -> handle {
  auto id = next_id_++;
  while (contains(id)) {
    id = next_id_++;
  }
  detail::component_table* table = nullptr;
  if (next_entity_table_index_ < entity_tables_.size()) {
    table = &entity_tables_[next_entity_table_index_];
  } else {
    table = &entity_tables_.emplace_back();
  }
  table->id = id;
  entities_.emplace(id, table);
  ++next_entity_table_index_;
  return {id, this, table};
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
    handle{id, this, it->second}.clear();
    it->second->id.reset();
    entities_.erase(it);
  }
}

inline auto EntityIndex::get(entity_id id) -> std::optional<handle> {
  std::optional<handle> r;
  if (auto it = entities_.find(id); it != entities_.end()) {
    r = {id, this, it->second};
  }
  return r;
}

inline auto EntityIndex::get(entity_id id) const -> std::optional<const_handle> {
  std::optional<const_handle> r;
  if (auto it = entities_.find(id); it != entities_.end()) {
    r = {id, this, it->second};
  }
  return r;
}

template <Component C>
index_type EntityIndex::count() const {
  if (auto* c = storage_get<C>(); c) {
    return c->size;
  }
  return 0;
}

template <Component C>
bool EntityIndex::has(entity_id id) const {
  auto h = get(id);
  return h && h->has<C>();
}

inline bool EntityIndex::has(entity_id id, component_id cid) const {
  auto h = get(id);
  return h && h->has(cid);
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
void EntityIndex::iterate(std::invocable<C&> auto&& f, bool include_new) {
  if (auto* c = storage_get<C>()) {
    detail::iterate(*c, include_new, [&](auto& e) { f(*e.data); });
  }
}

template <Component C>
void EntityIndex::iterate(std::invocable<const C&> auto&& f, bool include_new) const {
  if (auto* c = storage_get<C>(); c) {
    detail::iterate(*c, include_new, [&](const auto& e) { f(*e.data); });
  }
}

template <Component C>
void EntityIndex::iterate_dispatch(auto&& f, bool include_new) {
  if (auto* c = storage_get<C>(); c) {
    detail::iterate(*c, include_new, [&](auto& e) { dispatch(*get(e.id), f); });
  }
}

template <Component C>
void EntityIndex::iterate_dispatch(auto&& f, bool include_new) const {
  if (auto* c = storage_get<C>(); c) {
    detail::iterate(*c, include_new, [&](const auto& e) { dispatch(*get(e.id), f); });
  }
}

template <Component C>
void EntityIndex::iterate_dispatch_if(auto&& f, bool include_new) {
  if (auto* c = storage_get<C>(); c) {
    detail::iterate(*c, include_new, [&](auto& e) { dispatch_if(*get(e.id), f); });
  }
}

template <Component C>
void EntityIndex::iterate_dispatch_if(auto&& f, bool include_new) const {
  if (auto* c = storage_get<C>(); c) {
    detail::iterate(*c, include_new, [&](const auto& e) { dispatch_if(*get(e.id), f); });
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

template <Component C>
auto EntityIndex::storage_get() -> detail::component_storage_get<C>* {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < components_.size() && components_[c_id]
      ? static_cast<detail::component_storage_get<C>*>(components_[c_id].get())
      : nullptr;
}

template <Component C>
auto EntityIndex::storage_get() const -> const detail::component_storage_get<C>* {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < components_.size() && components_[c_id]
      ? static_cast<detail::component_storage_get<C>*>(components_[c_id].get())
      : nullptr;
}

template <bool Const>
template <Component C, typename... Args>
C& handle_base<Const>::emplace(Args&&... args) const
    requires(!Const) && std::constructible_from<C, Args...>
{
  auto& storage = index_->template storage<C>();
  auto index = table_->template get<C>();
  if (index) {
    auto& e = storage.entries[*index];
    e.data.emplace(std::forward<Args>(args)...);
    return *e.data;
  }
  ++storage.size;
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
C& handle_base<Const>::add(C&& data) const requires Component<std::remove_cvref_t<C>>
{
  return emplace<std::remove_cvref_t<C>>(std::forward<C>(data));
}

template <bool Const>
template <Component C>
void handle_base<Const>::remove() const requires(!Const)
{
  if (auto index = table_->template get<C>(); index) {
    auto& c = index_->template storage<C>();
    for (const auto& f : c.remove_callbacks()) {
      f(*this, *c.entries[*index].data);
    }
    table_->template reset<C>();
    c.entries[*index].data.reset();
    --c.size;
  }
}

template <bool Const>
void handle_base<Const>::clear() const requires(!Const)
{
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
bool handle_base<Const>::has(component_id cid) const {
  return table_->get(cid).has_value();
}

template <bool Const>
template <Component C>
C* handle_base<Const>::get() const requires(!Const)
{
  if (auto index = table_->template get<C>(); index) {
    return &*index_->template storage_get<C>()->entries[*index].data;
  }
  return nullptr;
}

template <bool Const>
template <Component C>
const C* handle_base<Const>::get() const requires Const
{
  if (auto index = table_->template get<C>(); index) {
    return &*index_->template storage_get<C>()->entries[*index].data;
  }
  return nullptr;
}

}  // namespace ii::ecs

#endif
