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
