#ifndef II_GAME_GEOM2_SHAPE_BANK_H
#define II_GAME_GEOM2_SHAPE_BANK_H
#include "game/geom2/resolve.h"
#include "game/geom2/shape_data.h"
#include <cstdint>
#include <deque>
#include <variant>
#include <vector>

namespace ii::geom2 {

class ShapeBank {
public:
  using node_data = std::variant<ball_collider, box_collider, ngon_collider, ball, box, ngon,
                                 translate, rotate, enable>;
  struct node_t {
    node_data data;
    std::vector<node_t*> children;
  };

  struct handle {
    ShapeBank* bank = nullptr;
    node_t* node = nullptr;

    handle add(const node_data& data) const {
      auto child = bank->add(data);
      add(child);
      return child;
    }

    void add(const handle& h) const { node->children.emplace_back(h.node); }
  };

  handle add(const node_data& data) { return {this, &data_.emplace_back(node_t{.data = data})}; }

private:
  std::deque<node_t> data_;
};

}  // namespace ii::geom2

#endif
