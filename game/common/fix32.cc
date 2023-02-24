#include "game/common/fix32.h"
#include <iomanip>
#include <sstream>

std::ostream& operator<<(std::ostream& o, const fixed& f) {
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << f.to_float();
  o << ss.str();
  return o;
}
