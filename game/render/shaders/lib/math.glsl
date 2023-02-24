const float kPi = 3.1415926538;

vec2 from_polar(float a, float r) {
  return vec2(r * cos(a), r * sin(a));
}

vec2 rotate(vec2 v, float a) {
  float c = cos(a);
  float s = sin(a);
  return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}
