#include "game/common/z.h"
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <map>

namespace {

static std::map<std::pair<colour_t, std::int32_t>, colour_t> cycle_map;

colour_t rgb_to_hsl(colour_t rgb) {
  double r = ((rgb >> 24) & 0xff) / 255.0;
  double g = ((rgb >> 16) & 0xff) / 255.0;
  double b = ((rgb >> 8) & 0xff) / 255.0;

  double vm, r2, g2, b2;
  double h = 0, s = 0, l = 0;

  double v = std::max(r, std::max(b, g));
  double m = std::min(r, std::min(b, g));
  l = (m + v) / 2.0;
  if (l <= 0.0) {
    return 0;
  }
  s = vm = v - m;
  if (s > 0.0) {
    s /= (l <= 0.5) ? (v + m) : (2.0 - v - m);
  } else {
    return ((static_cast<std::int32_t>(0.5 + l * 255) & 0xff) << 8);
  }
  r2 = (v - r) / vm;
  g2 = (v - g) / vm;
  b2 = (v - b) / vm;
  if (r == v) {
    h = (g == m ? 5.0 + b2 : 1.0 - g2);
  } else if (g == v) {
    h = (b == m ? 1.0 + r2 : 3.0 - b2);
  } else {
    h = (r == m ? 3.0 + g2 : 5.0 - r2);
  }
  h /= 6.0;
  return ((static_cast<std::int32_t>(0.5 + h * 255) & 0xff) << 24) |
      ((static_cast<std::int32_t>(0.5 + s * 255) & 0xff) << 16) |
      ((static_cast<std::int32_t>(0.5 + l * 255) & 0xff) << 8);
}

colour_t hsl_to_rgb(colour_t hsl) {
  double h = ((hsl >> 24) & 0xff) / 255.0;
  double s = ((hsl >> 16) & 0xff) / 255.0;
  double l = ((hsl >> 8) & 0xff) / 255.0;

  double r = l;
  double g = l;
  double b = l;
  double v = (l <= 0.5) ? l * (1.0 + s) : l + s - l * s;
  if (v > 0) {
    h *= 6.0;
    double m = l + l - v;
    double sv = (v - m) / v;
    std::int32_t sextant = static_cast<std::int32_t>(h);
    double vsf = v * sv * (h - sextant);
    double mid1 = m + vsf;
    double mid2 = v - vsf;
    sextant %= 6;

    switch (sextant) {
    case 0:
      r = v;
      g = mid1;
      b = m;
      break;
    case 1:
      r = mid2;
      g = v;
      b = m;
      break;
    case 2:
      r = m;
      g = v;
      b = mid1;
      break;
    case 3:
      r = m;
      g = mid2;
      b = v;
      break;
    case 4:
      r = mid1;
      g = m;
      b = v;
      break;
    case 5:
      r = v;
      g = m;
      b = mid2;
      break;
    }
  }
  return ((static_cast<std::int32_t>(0.5 + r * 255) & 0xff) << 24) |
      ((static_cast<std::int32_t>(0.5 + g * 255) & 0xff) << 16) |
      ((static_cast<std::int32_t>(0.5 + b * 255) & 0xff) << 8);
}

}  // namespace

namespace z {

std::int32_t state = 0;

void seed(std::int32_t seed) {
  state = seed;
}

std::int32_t rand_int() {
  static constexpr std::int32_t a = 1103515245;
  static constexpr std::int32_t c = 12345;
  state = a * state + c;
  return (state >> 16) & 0x7fff;
}

std::string crypt(const std::string& text, const std::string& key) {
  std::string result;
  for (std::size_t i = 0; i < text.length(); ++i) {
    char c = text[i] ^ key[i % key.length()];
    if (text[i] == '\0' || c == '\0') {
      result += text[i];
    } else {
      result += c;
    }
  }
  return result;
}

std::string compress_string(const std::string& str) {
  constexpr std::int32_t compression_level = Z_BEST_COMPRESSION;
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, compression_level) != Z_OK) {
    throw std::runtime_error("deflateInit failed while compressing.");
  }

  zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(str.data()));
  zs.avail_in = static_cast<uInt>(str.size());

  std::int32_t result = 0;
  char out_buffer[32768];
  std::string out_string;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
    zs.avail_out = sizeof(out_buffer);

    result = deflate(&zs, Z_FINISH);

    if (out_string.size() < zs.total_out) {
      out_string.append(out_buffer, zs.total_out - out_string.size());
    }
  } while (result == Z_OK);

  deflateEnd(&zs);

  if (result != Z_STREAM_END) {
    std::ostringstream ss;
    ss << "Exception during zlib compression: (" << result << ") " << zs.msg;
    throw std::runtime_error{ss.str()};
  }
  return out_string;
}

std::string decompress_string(const std::string& str) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK) {
    throw std::runtime_error("inflateInit failed while decompressing.");
  }

  zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(str.data()));
  zs.avail_in = static_cast<uInt>(str.size());

  std::int32_t result = 0;
  char out_buffer[32768];
  std::string out_string;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
    zs.avail_out = sizeof(out_buffer);

    result = inflate(&zs, 0);

    if (out_string.size() < zs.total_out) {
      out_string.append(out_buffer, zs.total_out - out_string.size());
    }
  } while (result == Z_OK);

  inflateEnd(&zs);

  if (result != Z_STREAM_END) {
    std::ostringstream ss;
    ss << "Exception during zlib decompression: (" << result << ") " << zs.msg;
    throw std::runtime_error{ss.str()};
  }
  return out_string;
}

colour_t colour_cycle(colour_t rgb, std::int32_t cycle) {
  if (!cycle) {
    return rgb;
  }
  colour_t a = rgb & 0x000000ff;
  auto key = std::make_pair(rgb & 0xffffff00, cycle);
  if (cycle_map.find(key) != cycle_map.end()) {
    return cycle_map[key] | a;
  }

  colour_t hsl = rgb_to_hsl(rgb & 0xffffff00);
  char c = ((hsl >> 24) & 0xff) + cycle;
  colour_t result = hsl_to_rgb((hsl & 0x00ffffff) | (c << 24));
  cycle_map[key] = result;
  return result | a;
}

}  // namespace z
