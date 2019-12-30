
out vec4 fragment_color;

in vec4 out_color;
in vec2 out_tex_coord;

uniform sampler2D color_texture;

void main() {
    fragment_color = out_color * texture(color_texture, out_tex_coord.st);
}

