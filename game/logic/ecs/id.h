#ifndef II_GAME_LOGIC_ECS_ID_H
#define II_GAME_LOGIC_ECS_ID_H
#include "game/common/enum.h"
#include <cstdint>
#include <type_traits>

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
enum class entity_id : index_type {};
enum class component_id : index_type {};

struct component {};
template <typename T>
concept Component = std::is_base_of_v<component, T>;

template <Component C>
component_id id() {
  return sequential_id<component_id>::value<C>;
}
}  // namespace ii::ecs

namespace ii {
template <>
struct integral_enum<ecs::entity_id> : std::true_type {};
template <>
struct integral_enum<ecs::component_id> : std::true_type {};
}  // namespace ii

#endif