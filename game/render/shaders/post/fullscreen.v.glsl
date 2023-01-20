uniform vec2 aspect_scale;

layout(location = 0) in vec2 in_position;

out vec2 v_texture_coords;

void main() {
  vec2 v = in_position * aspect_scale;
  v_texture_coords = v;
  gl_Position = vec4(v, 0., 1.);
}
