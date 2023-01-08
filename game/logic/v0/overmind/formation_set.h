#ifndef II_GAME_LOGIC_V0_OVERMIND_FORMATION_SET_H
#define II_GAME_LOGIC_V0_OVERMIND_FORMATION_SET_H
#include "game/logic/v0/overmind/spawn_context.h"
#include <sfn/functional.h>
#include <cstdint>
#include <vector>

namespace ii::v0 {

template <typename T>
concept Formation = requires(const T& t, spawn_context& context) {
                      T{};
                      t(context);
                      std::uint32_t{T::power_cost};
                      std::uint32_t{T::power_min};
                    };

class FormationSet {
private:
  struct entry_t {
    std::uint32_t weight = 0;
    std::uint32_t power_cost = 0;
    std::uint32_t power_min = 0;
    sfn::ptr<void(spawn_context&)> function = nullptr;
  };
  struct set_t {
    std::uint32_t weight = 0;
    std::vector<entry_t> entries;
  };

public:
  // TODO: somehow allow weighted entries that describe something like:
  // spend half of power budget on this set of formations, half on this other set.
  // OR maybe don't bother and just do it manually in callsites.
  class Handle {
  public:
    template <Formation F>
    void add(std::uint32_t weight = 1) {
      auto& e = set_->sets_[index_].entries.emplace_back();
      e.weight = weight;
      e.power_cost = F::power_cost;
      e.power_min = F::power_min;
      e.function = +[](spawn_context& context) {
        F f{};
        static_cast<const F&>(f)(context);
      };
    }

  private:
    friend class FormationSet;
    Handle(FormationSet* set, std::size_t index) : set_{set}, index_{index} {}
    FormationSet* set_ = nullptr;
    std::size_t index_ = 0;
  };

  Handle add_set(std::uint32_t weight = 1) {
    auto index = sets_.size();
    auto& set = sets_.emplace_back();
    set.weight = weight;
    return {this, index};
  }

  void spawn_wave(spawn_context& context) const {
    auto it = context.random.weighted_choice(sets_.begin(), sets_.end(),
                                             [&](const auto& set) { return set.weight; });
    if (it != sets_.end()) {
      spawn_wave(*it, context);
    }
  }

private:
  static void spawn_wave(const set_t& set, spawn_context& context) {
    std::vector<const entry_t*> chosen_entries;
    auto power_budget = context.data.power;
    while (power_budget) {
      auto it = context.random.weighted_choice(
          set.entries.begin(), set.entries.end(), [&](const auto& e) {
            return context.data.power >= e.power_min && power_budget >= e.power_cost ? e.weight
                                                                                     : 0u;
          });
      if (it == set.entries.end()) {
        break;
      }
      auto i = context.random.uint(1 + chosen_entries.size());
      chosen_entries.insert(chosen_entries.begin() + i, &*it);
      power_budget -= it->power_cost;
    }

    context.row_number = 0;
    for (const auto* e : chosen_entries) {
      e->function(context);
      ++context.row_number;
    }
  }

  std::vector<set_t> sets_;
};

}  // namespace ii::v0

#endif
