#ifndef II_GAME_GEOMETRY_SHAPE_BANK_H
#define II_GAME_GEOMETRY_SHAPE_BANK_H
#include "game/geometry/resolve.h"
#include "game/geometry/shape_data.h"
#include "game/geometry/value_parameters.h"
#include <sfn/functional.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ii::geom {

enum class node_type : std::uint32_t {
  kNone = 0x0,
  kCollision = 0x1,
  kResolve = 0x2,
};

}  // namespace ii::geom

namespace ii {
template <>
struct bitmask_enum<geom::node_type> : std::true_type {};
}  // namespace ii

namespace ii::geom {

class ShapeBank {
public:
  ShapeBank() = default;
  ShapeBank(ShapeBank&&) = delete;
  ShapeBank(const ShapeBank&) = delete;
  ShapeBank& operator=(ShapeBank&&) = delete;
  ShapeBank& operator=(const ShapeBank&) = delete;

  using node_data =
      std::variant<ball_collider, box_collider, ngon_collider, arc_collider, ball, box, line, ngon,
                   compound, enable, translate, rotate, translate_rotate>;

  class node {
  private:
    struct access_tag {};

  public:
    node(node&&) = delete;
    node(const node&) = delete;
    node& operator=(node&&) = delete;
    node& operator=(const node&) = delete;
    node(access_tag, ShapeBank* bank, const node_data& data) : bank_{bank}, data_{data} {}

    std::size_t size() const { return children_.size(); }
    node& operator[](std::size_t i) { return *children_[i]; }
    const node& operator[](std::size_t i) const { return *children_[i]; }

    std::size_t collision_size() const { return collision_.size(); }
    node& collision(std::size_t i) { return *collision_[i]; }
    const node& collision(std::size_t i) const { return *collision_[i]; }

    const node_data& operator*() const { return data_; }
    const node_data* operator->() const { return &data_; }
    node& create(const node_data& data) { return bank_->add(data); }
    void add(node& child) { children_.emplace_back(&child); }

    node& add(const node_data& data) {
      auto& child = create(data);
      add(child);
      return child;
    }

  private:
    friend class ShapeBank;
    ShapeBank* bank_ = nullptr;
    node_data data_;
    std::vector<node*> children_;
    std::vector<node*> collision_;
  };

  using node_construct_t = sfn::ptr<void(node&)>;
  const node& operator[](node_construct_t construct) {
    if (auto it = map_.find(construct); it != map_.end()) {
      return *it->second;
    }
    auto& node = add(compound{});
    construct(node);
    optimize(node);
    map_[construct] = &node;
    return node;
  }

  template <typename F>
  parameter_set& parameters(F&& set_function) {
    set_function(parameters_);
    return parameters_;
  }

private:
  static node_type optimize(node&);
  node& add(const node_data& data) { return {data_.emplace_back(node::access_tag{}, this, data)}; }

  parameter_set parameters_;
  std::deque<node> data_;
  std::unordered_map<node_construct_t, node*> map_;
};

using node = ShapeBank::node;
void resolve(resolve_result&, const node&, const parameter_set&);
void check_collision(hit_result&, const check_t&, const node&, const parameter_set&);

template <typename F>
inline void resolve(resolve_result& result, ShapeBank& shape_bank,
                    ShapeBank::node_construct_t construct, F&& set_function) {
  resolve(result, shape_bank[construct], shape_bank.parameters(set_function));
}

template <typename F>
inline void check_collision(hit_result& result, const check_t& check, ShapeBank& shape_bank,
                            ShapeBank::node_construct_t construct, F&& set_function) {
  check_collision(result, check, shape_bank[construct], shape_bank.parameters(set_function));
}

}  // namespace ii::geom

#endif
