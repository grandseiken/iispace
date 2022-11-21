#include "game/core/sim/hud_layer.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/logic/sim/io/output.h"

namespace ii {
inline std::string convert_to_time(std::uint64_t score) {
  if (!score) {
    return "--:--";
  }
  score /= 60u;
  std::uint64_t secs = score % 60u;
  score /= 60u;
  std::uint64_t mins = score % 60u;
  score /= 60u;

  std::string r;
  if (score) {
    r += std::to_string(score) + ":";
  }
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

class PlayerHud : public ui::Panel {
public:
  PlayerHud(ui::Element* parent, ui::alignment align) : ui::Panel(parent) {
    auto& layout = *add_back<ui::LinearLayout>();
    layout.set_spacing(kPadding.y);
    if (+(align & ui::alignment::kTop)) {
      status_ = layout.add_back<ui::TextElement>();
      debug_ = layout.add_back<ui::TextElement>();
    } else {
      debug_ = layout.add_back<ui::TextElement>();
      status_ = layout.add_back<ui::TextElement>();
    }
    layout.set_align_end(!(align & ui::alignment::kTop));
    layout.set_absolute_size(*status_, kMediumFont.y);

    status_->set_alignment(align)
        .set_font(render::font_id::kMonospace)
        .set_font_dimensions(kMediumFont)
        .set_colour(kTextColour);
    debug_->set_alignment(align)
        .set_font(render::font_id::kMonospace)
        .set_font_dimensions(kSmallFont)
        .set_colour(kTextColour)
        .set_multiline(true);
  }

  void set_colour(const glm::vec4& colour) { status_->set_colour(colour); }

  void set_player_status(ustring&& s) { status_->set_text(std::move(s)); }

  void set_debug_text(ustring&& s) { debug_->set_text(std::move(s)); }

private:
  ui::TextElement* status_ = nullptr;
  ui::TextElement* debug_ = nullptr;
};

HudLayer::HudLayer(ui::GameStack& stack, game_mode mode)
: ui::GameLayer{stack, ui::layer_flag::kCaptureCursor}, mode_{mode} {
  set_bounds(rect{kUiDimensions});

  static constexpr auto padding = 2u * kPadding;
  auto add = [&](const rect& r, ui::alignment align) {
    auto& p = *add_back<PlayerHud>(align);
    p.set_margin(padding);
    p.set_bounds(r);
    return &p;
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

  auto& p = *add_back<ui::Panel>();
  p.set_margin(padding);
  p.set_bounds(bounds().size_rect());
  status_ = p.add_back<ui::TextElement>();
  status_->set_alignment(ui::alignment::kBottom)
      .set_font(render::font_id::kMonospace)
      .set_font_dimensions(kMediumFont)
      .set_multiline(true);
  if (!is_legacy_mode(mode)) {
    status_->set_drop_shadow(kDropShadow, .5f);
  }
}

void HudLayer::update_content(const ui::input_frame&, ui::output_frame&) {
  if (!render_) {
    return;
  }
  for (std::size_t i = 0; i < 4; ++i) {
    if (i >= render_->players.size()) {
      huds_[i]->set_player_status({});
      huds_[i]->set_debug_text({});
      continue;
    }
    huds_[i]->set_debug_text(ustring::ascii(debug_text_[i]));
    if (mode_ == game_mode::kStandardRun) {
      continue;
    }
    const auto& p = render_->players[i];
    huds_[i]->set_colour(p.colour);
    huds_[i]->set_player_status(
        ustring::ascii(std::to_string(p.score) + " (" + std::to_string(p.multiplier) + "X)"));
  }

  std::string s;
  if (mode_ == game_mode::kStandardRun) {
    s = convert_to_time(render_->tick_count);
    if (render_->overmind_wave) {
      s += " (wave " + std::to_string(*render_->overmind_wave) + ")";
    }
  } else {
    s = std::to_string(render_->lives_remaining) + " live(s)";
    if (mode_ == game_mode::kLegacy_Boss) {
      s += "\n" + convert_to_time(render_->tick_count);
    } else if (render_->overmind_timer) {
      auto t = *render_->overmind_timer / 60;
      s += (t < 10 ? " 0" : " ") + std::to_string(t);
    }
  }
  if (!status_text_.empty()) {
    s = status_text_ + "\n" + s;
  }
  if (!render_->debug_text.empty() && stack().options().debug) {
    s += "\n" + render_->debug_text;
  }
  status_->set_text(ustring::ascii(s));
}

}  // namespace ii
