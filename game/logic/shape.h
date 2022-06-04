#ifndef II_GAME_LOGIC_SHAPE_H
#define II_GAME_LOGIC_SHAPE_H
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace ii {
class SimInterface;
}  // namespace ii

class Shape {
public:
  Shape(const vec2& centre, fixed rotation, const glm::vec4& colour, std::uint32_t category,
        bool can_rotate = true);
  virtual ~Shape() {}

  bool check_point(const vec2& v) const;
  vec2 convert_point(const vec2& position, fixed rotation, const vec2& v) const;
  glm::vec2 convert_fl_point(const glm::vec2& position, float rotation, const glm::vec2& v) const;

  fixed rotation() const;
  void set_rotation(fixed rotation);
  void rotate(fixed rotation_amount);

  virtual void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                      const std::optional<glm::vec4>& colour_override = std::nullopt) const = 0;

  vec2 centre;
  glm::vec4 colour{0.f};
  std::uint32_t category = 0;

private:
  virtual bool check_local_point(const vec2& v) const = 0;

  fixed rotation_ = 0;
  bool can_rotate_ = false;
};

class Fill : public Shape {
public:
  Fill(const vec2& centre, fixed width, fixed height, const glm::vec4& colour,
       std::uint32_t category = 0);

  void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

  fixed width = 0;
  fixed height = 0;

private:
  bool check_local_point(const vec2& v) const override;
};

class Line : public Shape {
public:
  Line(const vec2& centre, const vec2& a, const vec2& b, const glm::vec4& colour,
       fixed rotation = 0);

  void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

  vec2 a;
  vec2 b;

private:
  bool check_local_point(const vec2& v) const override;
};

class Box : public Shape {
public:
  Box(const vec2& centre, fixed width, fixed height, const glm::vec4& colour, fixed rotation = 0,
      std::uint32_t category = 0);

  void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

  fixed width = 0;
  fixed height = 0;

private:
  bool check_local_point(const vec2& v) const override;
};

class Polygon : public Shape {
public:
  enum class T {
    kPolygon,
    kPolystar,
    kPolygram,
  };

  Polygon(const vec2& centre, fixed radius, std::uint32_t sides, const glm::vec4& colour,
          fixed rotation = 0, std::uint32_t category = 0, T type = T::kPolygon);

  void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

  fixed radius = 0;
  std::uint32_t sides = 0;
  T type = T::kPolygon;

private:
  bool check_local_point(const vec2& v) const override;
};

class PolyArc : public Shape {
public:
  PolyArc(const vec2& centre, fixed radius, std::uint32_t sides, std::uint32_t segments,
          const glm::vec4& colour, fixed rotation = 0, std::uint32_t category = 0);

  void render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

  fixed radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = 0;

private:
  bool check_local_point(const vec2& v) const override;
};

class CompoundShape : public Shape {
public:
  CompoundShape(const vec2& centre, fixed rotation = 0, std::uint32_t category = 0);

  typedef std::vector<std::unique_ptr<Shape>> shape_list;
  const shape_list& shapes() const;

  template <typename T, typename... Args>
  T* add_new_shape(Args&&... args) {
    return static_cast<T*>(add_shape(std::make_unique<T>(std::forward<Args>(args)...)));
  }

  Shape* add_shape(std::unique_ptr<Shape> shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

  void render(ii::SimInterface& sim, const glm::vec2& position, float rot,
              const std::optional<glm::vec4>& colour_override = std::nullopt) const override;

private:
  bool check_local_point(const vec2& v) const override;

  shape_list children_;
};

#endif
