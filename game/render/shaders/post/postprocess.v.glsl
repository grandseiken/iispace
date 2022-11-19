layout(location = 0) in vec2 in_position;

out vec2 v_texture_coords;

void main() {
  v_texture_coords = in_position;
  gl_Position = vec4(in_position.xy, 0., 1.);
}
