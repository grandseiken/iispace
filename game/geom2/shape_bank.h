#ifndef II_GAME_GEOM2_SHAPE_BANK_H
#define II_GAME_GEOM2_SHAPE_BANK_H
#include "game/geom2/resolve.h"
#include "game/geom2/shape_data.h"
#include "game/geom2/value_parameters.h"
#include <cstdint>
#include <deque>
#include <variant>
#include <vector>

namespace ii::geom2 {

class ShapeBank {
public:
  using node_data = std::variant<ball_collider, box_collider, ngon_collider, ball, box, line, ngon,
                                 translate, rotate, enable>;

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

  node& add(const node_data& data) { return {data_.emplace_back(node::access_tag{}, this, data)}; }

private:
  std::deque<node> data_;
};

void resolve(resolve_result&, const ShapeBank::node&, const parameter_set&);
void check_collision(hit_result&, const ShapeBank::node&, const parameter_set&, const check_t&);

}  // namespace ii::geom2

#endif
