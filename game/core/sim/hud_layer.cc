#include "game/core/sim/hud_layer.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/logic/sim/io/output.h"

namespace ii {
inline std::string convert_to_time(std::uint64_t score) {
  if (!score) {
    return "--:--";
  }
  std::uint64_t mins = 0;
  while (score >= 60ull * 60ull && mins < 99u) {
    score -= 60ull * 60ull;
    ++mins;
  }
  std::uint64_t secs = score / 60u;

  std::string r;
  if (mins < 10u) {
    r += '0';
  }
  r += std::to_string(mins) + ":";
  if (secs < 10u) {
    r += '0';
  }
  r += std::to_string(secs);
  return r;
}

HudLayer::HudLayer(ui::GameStack& stack, game_mode mode)
: ui::GameLayer{stack, ui::layer_flag::kCaptureCursor}, mode_{mode} {
  set_bounds(rect{kUiDimensions});

  auto add = [&](const rect& r, ui::alignment align) {
    auto& p = *add_back<ui::Panel>();
    p.set_bounds(r);
    p.set_margin(2u * kPadding);
    auto& t = *p.add_back<ui::TextElement>();
    t.set_alignment(align).set_font(render::font_id::kMonospace).set_font_dimensions(kMediumFont);
    return &t;
  };

  huds_ = {
      add(rect{glm::ivec2{0}, kUiDimensions / 2}, ui::alignment::kLeft | ui::alignment::kTop),
      add(rect{glm::ivec2{kUiDimensions.x / 2, 0}, kUiDimensions / 2},
          ui::alignment::kRight | ui::alignment::kTop),
      add(rect{glm::ivec2{0, kUiDimensions.y / 2}, kUiDimensions / 2},
          ui::alignment::kLeft | ui::alignment::kBottom),
      add(rect{kUiDimensions / 2, kUiDimensions / 2},
          ui::alignment::kRight | ui::alignment::kBottom),
  };
  status_ = add(bounds().size_rect(), ui::alignment::kBottom);
  status_->set_multiline(true);
}

void HudLayer::update_content(const ui::input_frame&, ui::output_frame&) {
  if (!render_) {
    return;
  }
  for (std::size_t i = 0; i < 4; ++i) {
    if (i >= render_->players.size()) {
      huds_[i]->set_text(ustring::ascii(""));
      continue;
    }
    const auto& p = render_->players[i];
    huds_[i]->set_colour(p.colour);
    huds_[i]->set_text(
        ustring::ascii(std::to_string(p.score) + " (" + std::to_string(p.multiplier) + "X)"));
  }

  std::string s = std::to_string(render_->lives_remaining) + " live(s)";
  if (mode_ == game_mode::kBoss) {
    s += "\n" + convert_to_time(render_->tick_count);
  } else if (render_->overmind_timer) {
    auto t = *render_->overmind_timer / 60;
    s += (t < 10 ? " 0" : " ") + std::to_string(t);
  }
  if (!status_text_.empty()) {
    s = status_text_ + "\n" + s;
  }
  status_->set_text(ustring::ascii(s));
}

}  // namespace ii