#define _USE_MATH_DEFINES
#include <iomanip>
#include <cmath>
#include <fstream>

#define NUM (65536)

int main()
{
  std::ofstream f("lookup.h");
  f << std::fixed << std::setprecision(24);

  f << "static const float SIN_LOOKUP[" << NUM << "] = {\n    ";
  for (int i = 0; i < NUM; ++i) {
    f << sin(float(2.0 * M_PI * double(i) / double(NUM))) << "f,";
    if (i != 0 && i % 16 == 0) {
      f << "\n    ";
    }
    else {
      f << " ";
    }
  }
  f << "};\n";

  f << "static const float COS_LOOKUP[" << NUM << "] = {\n    ";
  for (int i = 0; i < NUM; ++i) {
    f << cos(float(2.0 * M_PI * double(i) / double(NUM))) << "f,";
    if (i != 0 && i % 16 == 0) {
      f << "\n    ";
    }
    else {
      f << " ";
    }
  }
  f << "};\n";

  f << "static const float ATAN_LOOKUP[" << NUM << "] = {\n    ";
  for (int i = 0; i < NUM; ++i) {
    f << atan(float(double(i) / double(NUM))) << "f,";
    if (i != 0 && i % 16 == 0) {
      f << "\n    ";
    }
    else {
      f << " ";
    }
  }
  f << "};";
  f.close();
}