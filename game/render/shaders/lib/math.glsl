const float kPi = 3.1415926538;

vec2 from_polar(float a, float r) {
  return vec2(r * cos(a), r * sin(a));
}

vec2 rotate(vec2 v, float a) {
  float c = cos(a);
  float s = sin(a);
  return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

float aa_step(bool aa, float threshold, float v) {
  return aa ? smoothstep(threshold - fwidth(v), threshold, v) : step(threshold, v);
}

float aa_step2(bool aa, float threshold, float v) {
  if (aa) {
    float w = fwidth(v);
    return smoothstep(threshold - w, threshold + w, v);
  }
  return step(threshold, v);
}