#include "z.h"
#include <algorithm>
#include <map>
#include <zlib.h>

namespace {

static std::map<std::pair<colour, int32_t>, colour> cycle_map;

colour rgb_to_hsl(colour rgb)
{
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
  }
  else {
    return ((int(0.5 + l * 255) & 0xff) << 8);
  }
  r2 = (v - r) / vm;
  g2 = (v - g) / vm;
  b2 = (v - b) / vm;
  if (r == v) {
    h = (g == m ? 5.0 + b2 : 1.0 - g2);
  }
  else if (g == v) {
    h = (b == m ? 1.0 + r2 : 3.0 - b2);
  }
  else {
    h = (r == m ? 3.0 + g2 : 5.0 - r2);
  }
  h /= 6.0;
  return
      ((int(0.5 + h * 255) & 0xff) << 24) |
      ((int(0.5 + s * 255) & 0xff) << 16) |
      ((int(0.5 + l * 255) & 0xff) << 8);
}

colour hsl_to_rgb(colour hsl)
{
  double h = ((hsl >> 24) & 0xff) / 255.0;
  double s = ((hsl >> 16) & 0xff) / 255.0;
  double l = ((hsl >> 8) & 0xff) / 255.0;

  double r = l, g = l, b = l;
  double v = (l <= 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
  if (v > 0) {
    h *= 6.0;
    double m = l + l - v;
    double sv = (v - m) / v;;
    int sextant = int(h);
    double vsf = v * sv * (h - sextant);
    double mid1 = m + vsf, mid2 = v - vsf;
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
  return
      ((int(0.5 + r * 255) & 0xff) << 24) |
      ((int(0.5 + g * 255) & 0xff) << 16) |
      ((int(0.5 + b * 255) & 0xff) << 8);
}

// End anonymous namespace.
}

namespace z {

int32_t state = 0;

void seed(int32_t seed)
{
  state = seed;
}

int32_t rand_int()
{
  int32_t const a = 1103515245;
  int32_t const c = 12345;
  state = a * state + c;
  return (state >> 16) & 0x7fff;
}

std::string compress_string(const std::string& str)
{
  const int compressionLevel = Z_BEST_COMPRESSION;
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, compressionLevel) != Z_OK) {
    throw std::runtime_error("deflateInit failed while compressing.");
  }

  zs.next_in = (Bytef*) str.data();
  zs.avail_in = uInt(str.size());

  int ret;
  char outbuffer[32768];
  std::string outstring;

  do {
    zs.next_out = reinterpret_cast< Bytef* >(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = deflate(&zs, Z_FINISH);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
    throw std::runtime_error(oss.str());
  }

  return outstring;
}

std::string decompress_string(const std::string& str)
{
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK) {
    throw std::runtime_error("inflateInit failed while decompressing.");
  }

  zs.next_in = (Bytef*) str.data();
  zs.avail_in = uInt(str.size());

  int ret;
  char outbuffer[32768];
  std::string outstring;

  do {
    zs.next_out = reinterpret_cast< Bytef* >(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
    throw std::runtime_error(oss.str());
  }

  return outstring;
}

colour colour_cycle(colour rgb, int32_t cycle)
{
  if (cycle == 0)
    return rgb;
  int a = rgb & 0x000000ff;
  std::pair<colour, int> key = std::make_pair(rgb & 0xffffff00, cycle);
  if (cycle_map.find(key) != cycle_map.end()) {
    return cycle_map[key] | a;
  }

  colour hsl = rgb_to_hsl(rgb & 0xffffff00);
  char c = ((hsl >> 24) & 0xff) + cycle;
  colour result = hsl_to_rgb((hsl & 0x00ffffff) | (c << 24));
  cycle_map[key] = result;
  return result | a;
}

// End namespace z.
}