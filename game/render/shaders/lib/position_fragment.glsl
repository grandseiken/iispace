uniform vec2 aspect_scale;
uniform uvec2 render_dimensions;
uniform uvec2 screen_dimensions;

vec2 game_position(vec4 frag_coords) {
  return vec2(render_dimensions) *
      ((2. * frag_coords.xy / vec2(screen_dimensions) - 1.) /
           vec2(aspect_scale.x, -aspect_scale.y) +
       1.) /
      2.;
}