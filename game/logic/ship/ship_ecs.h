
#ifndef II_GAME_LOGIC_SHIP_SHIP_ECS_H
#define II_GAME_LOGIC_SHIP_SHIP_ECS_H
#include "game/common/enum.h"
#include <cstddef>
#include <cstdint>
#include <deque>
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
enum class ship_id : index_type {};
enum class component_id : index_type {};
}  // namespace ii::ecs

namespace ii {
template <>
struct integral_enum<ecs::ship_id> : std::true_type {};
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

class ShipIndex {
private:
  struct component_table;

  template <bool Const>
  class handle_base {
  public:
    ship_id id() const {
      return id_;
    }

    template <Component C, typename... Args>
    C& emplace(Args&&... args) const requires !Const;
    template <Component C>
    void remove() const requires !Const;
    void clear() const requires !Const;

    template <Component C>
    bool has() const;
    template <Component C>
    C* get() const requires !Const;
    template <Component C>
    const C* get() const requires Const;

  private:
    friend class ShipIndex;
    using index_t = std::conditional_t<Const, const ShipIndex, ShipIndex>;
    using table_t = std::conditional_t<Const, const component_table, component_table>;

    handle_base(ship_id s_id, index_t* index, table_t* table)
    : id_{s_id}, index_{index}, table_{table} {}

    ship_id id_;
    index_t* index_;
    table_t* table_;
  };

public:
  using handle = handle_base<false>;
  using const_handle = handle_base<true>;

  handle create();
  void destroy(ship_id id);
  void compact();

  bool contains(ship_id id) const {
    return ships_.contains(id);
  }
  std::optional<handle> get(ship_id id);
  std::optional<const_handle> get(ship_id id) const;

  template <Component C>
  bool has(ship_id id) const {
    auto h = get(id);
    return h && h->has<C>();
  }

  template <Component C>
  C* get(ship_id id) {
    auto h = get(id);
    return h ? h->get<C>() : nullptr;
  }

  template <Component C>
  const C* get(ship_id id) const {
    auto h = get(id);
    return h ? h->get<C>() : nullptr;
  }

private:
  struct component_table;
  struct component_storage_base {
    virtual ~component_storage_base() {}
    virtual void compact() = 0;
    virtual void reset_index(std::size_t index) = 0;
  };

  template <Component C>
  struct component_storage;
  template <Component C>
  component_storage<C>& storage();
  template <Component C>
  const component_storage<C>* storage() const;

  ship_id next_id_{0};
  std::unordered_map<ship_id, std::unique_ptr<component_table>> ships_;
  std::vector<std::unique_ptr<component_storage_base>> components_;
};

struct ShipIndex::component_table {
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
struct ShipIndex::component_storage : ShipIndex::component_storage_base {
  struct entry {
    ship_id id;
    std::optional<C> data;
  };
  std::deque<entry> entries;

  void compact() override;
  void reset_index(std::size_t index) override {
    entries[index].data.reset();
  }
};

inline auto ShipIndex::create() -> handle {
  // TODO: is ID reuse OK?
  auto id = next_id_++;
  while (contains(id)) {
    id = next_id_++;
  }
  auto [it, _] = ships_.emplace(id, std::make_unique<component_table>());
  return {id, this, it->second.get()};
}

inline void ShipIndex::destroy(ship_id id) {
  auto it = ships_.find(id);
  if (it != ships_.end()) {
    handle{id, this, it->second.get()}.clear();
    ships_.erase(it);
  }
}

inline void ShipIndex::compact() {
  for (const auto& c : components_) {
    if (c) {
      c->compact();
    }
  }
}

inline auto ShipIndex::get(ship_id id) -> std::optional<handle> {
  std::optional<handle> r;
  if (auto it = ships_.find(id); it != ships_.end()) {
    r = {id, this, it->second.get()};
  }
  return r;
}

inline auto ShipIndex::get(ship_id id) const -> std::optional<const_handle> {
  std::optional<const_handle> r;
  if (auto it = ships_.find(id); it != ships_.end()) {
    r = {id, this, it->second.get()};
  }
  return r;
}

template <Component C>
auto ShipIndex::storage() -> component_storage<C>& {
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
auto ShipIndex::storage() const -> const component_storage<C>* {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < components_.size() && components_[c_id]
      ? static_cast<component_storage<C>*>(components_[c_id].get())
      : nullptr;
}

template <bool Const>
template <Component C, typename... Args>
C& ShipIndex::handle_base<Const>::emplace(Args&&... args) const requires !Const {
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
void ShipIndex::handle_base<Const>::remove() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    table_->clear<C>();
    index_->storage<C>().entres[*index].data.reset();
  }
}

template <bool Const>
void ShipIndex::handle_base<Const>::clear() const requires !Const {
  for (std::size_t i = 0; i < table_->v.size(); ++i) {
    if (table_->v[i]) {
      index_->components_[i]->reset_index(*table_->v[i]);
    }
  }
  table_->v.clear();
}

template <bool Const>
template <Component C>
bool ShipIndex::handle_base<Const>::has() const {
  return table_->get<C>().has_value();
}

template <bool Const>
template <Component C>
C* ShipIndex::handle_base<Const>::get() const requires !Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>().entries[*index].data;
  }
  return nullptr;
}

template <bool Const>
template <Component C>
const C* ShipIndex::handle_base<Const>::get() const requires Const {
  if (auto index = table_->get<C>(); index) {
    return &*index_->storage<C>()->entries[*index].data;
  }
  return nullptr;
}

template <Component C>
void ShipIndex::component_storage<C>::compact() {
  // TODO
}

}  // namespace ii::ecs

#endif