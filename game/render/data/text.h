#ifndef II_GAME_RENDER_DATA_TEXT_H
#define II_GAME_RENDER_DATA_TEXT_H
#include <cstdint>

namespace ii::render {

enum class font_id : std::uint32_t {
  kDefault,
  kMonospace = kDefault,
  kMonospaceBold,
  kMonospaceItalic,
  kMonospaceBoldItalic,
};

}  // namespace ii::render

#endif