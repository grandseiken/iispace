
#ifndef II_GAME_LOGIC_SHIP_SHIP_ECS_H
#define II_GAME_LOGIC_SHIP_SHIP_ECS_H
#include "game/common/enum.h"
#include <cstddef>
#include <cstdint>
#include <limits>
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
  static constexpr std::size_t kMaxShips =
      static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<ship_id>>::max());

  using handle = handle_base<false>;
  using const_handle = handle_base<true>;

  std::optional<handle> create();
  void destroy(ship_id id);
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
  struct component_table {
    std::vector<std::optional<index_type>> v;
  };

  struct component_storage_base {
    virtual ~component_storage_base() {}
  };

  template <Component C>
  struct component_storage : component_storage_base {
    struct entry {
      ship_id id;
      std::optional<C> data;
    };
    std::vector<entry> entries;
  };

  template <Component C>
  component_storage<C>& storage();
  template <Component C>
  const component_storage<C>* storage() const;

  ship_id next_id_{0};
  std::unordered_map<ship_id, component_table> ships_;
  std::vector<std::unique_ptr<component_storage_base>> components_;
};

inline auto ShipIndex::create() -> std::optional<handle> {
  if (ships_.size() >= kMaxShips) {
    return std::nullopt;
  }
  // TODO: is ID reuse OK?
  auto id = next_id_++;
  while (contains(id)) {
    id = next_id_++;
  }
  auto pair = ships_.emplace(id, component_table{});
  return {{id, this, &pair.first->second}};
}

inline void ShipIndex::destroy(ship_id id) {
  // TODO.
  auto it = ships_.find(id);
  if (it != ships_.end()) {
    ships_.erase(it);
  }
}

inline auto ShipIndex::get(ship_id id) -> std::optional<handle> {
  std::optional<handle> r;
  if (auto it = ships_.find(id); it != ships_.end()) {
    r = {id, this, &it->second};
  }
  return r;
}

inline auto ShipIndex::get(ship_id id) const -> std::optional<const_handle> {
  std::optional<const_handle> r;
  if (auto it = ships_.find(id); it != ships_.end()) {
    r = {id, this, &it->second};
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
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  auto& storage = index_->storage<C>();
  if (c_id >= table_->v.size()) {
    table_->v.resize(1 + c_id);
  }
  if (!table_->v[c_id]) {
    table_->v[c_id] = storage.entries.size();
    storage.entries.emplace_back();
  }
  auto& e = storage.entries[*table_->v[c_id]];
  e.data.emplace(std::forward<Args>(args)...);
  return *e.data;
}

template <bool Const>
template <Component C>
void ShipIndex::handle_base<Const>::remove() const requires !Const {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  if (c_id >= table_->v.size() || !table_->v[c_id]) {
    return;
  }
  auto& storage = index_->storage<C>();
  auto c_index = *table_->v[c_id];
  table_->v[c_id].reset();
  if (1 + c_index < storage.entries.size()) {
    std::swap(storage.entries[c_index], storage.entries.back());
    index_->ships_[storage.entries[c_index].id].v[c_id] = c_index;
  }
  storage.entries.pop_back();
}

template <bool Const>
void ShipIndex::handle_base<Const>::clear() const requires !Const {
  // TODO
}

template <bool Const>
template <Component C>
bool ShipIndex::handle_base<Const>::has() const {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < table_->size() && table_->v[c_id];
}

template <bool Const>
template <Component C>
C* ShipIndex::handle_base<Const>::get() const requires !Const {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  return c_id < table_->size() && table_->v[c_id]
      ? &*index_->storage<C>().entries[*table_->v[c_id]].data
      : nullptr;
}

template <bool Const>
template <Component C>
const C* ShipIndex::handle_base<Const>::get() const requires Const {
  auto c_id = static_cast<std::size_t>(ecs::id<C>());
  const auto* storage = index_->storage<C>();
  return storage && c_id < table_->size() && table_->v[c_id]
      ? storage->entries[*table_->v[c_id]].data
      : nullptr;
}

}  // namespace ii::ecs

#endif