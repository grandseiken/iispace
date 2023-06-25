// Anti-aliased (if enabled) step from 0 to 1, reaching 1 at threshold value.
float aa_step_upper(bool aa, float threshold, float v) {
  return aa ? smoothstep(threshold - fwidth(v), threshold, v) : step(threshold, v);
}

// Anti-aliased (if enabled) step from 0 to 1, starting at 0 at threshold value.
float aa_step_lower(bool aa, float threshold, float v) {
  return aa ? smoothstep(threshold, threshold + fwidth(v), v) : step(threshold, v);
}

// Anti-alised (if enablep) step from 0 to 1, centred on threshold value.
float aa_step_centred(bool aa, float threshold, float v) {
  if (aa) {
    float w = fwidth(v);
    return smoothstep(threshold - w, threshold + w, v);
  }
  return step(threshold, v);
}
