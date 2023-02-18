#ifndef II_GAME_GEOM2_SHAPE_BANK_H
#define II_GAME_GEOM2_SHAPE_BANK_H
#include "game/geom2/resolve.h"
#include "game/geom2/shape_data.h"
#include "game/geom2/value_parameters.h"
#include <sfn/functional.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ii::geom2 {

class ShapeBank {
public:
  ShapeBank() = default;
  ShapeBank(ShapeBank&&) = delete;
  ShapeBank(const ShapeBank&) = delete;
  ShapeBank& operator=(ShapeBank&&) = delete;
  ShapeBank& operator=(const ShapeBank&) = delete;

  using node_data = std::variant<ball_collider, box_collider, ngon_collider, ball, box, line, ngon,
                                 compound, enable, translate, rotate>;

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
    const node_data& operator*() const { return data_; }
    const node_data* operator->() const { return &data_; }
    void add(node& child) { children_.emplace_back(&child); }

    node& add(const node_data& data) {
      auto& child = bank_->add(data);
      add(child);
      return child;
    }

  private:
    friend class ShapeBank;
    ShapeBank* bank_ = nullptr;
    node_data data_;
    std::vector<node*> children_;
  };

  using node_construct_t = sfn::ptr<void(node&)>;
  const node& operator[](node_construct_t construct) {
    if (auto it = map_.find(construct); it != map_.end()) {
      return *it->second;
    }
    auto& node = add(compound{});
    construct(node);
    map_[construct] = &node;
    return node;
  }

  template <typename F>
  parameter_set& parameters(F&& set_function) {
    parameters_.clear();
    set_function(parameters_);
    return parameters_;
  }

  node& add(const node_data& data) { return {data_.emplace_back(node::access_tag{}, this, data)}; }

private:
  parameter_set parameters_;
  std::deque<node> data_;
  std::unordered_map<node_construct_t, node*> map_;
};

using node = ShapeBank::node;
void resolve(resolve_result&, const node&, const parameter_set&);
void check_collision(hit_result&, const node&, const parameter_set&, const check_t&);

}  // namespace ii::geom2

#endif
