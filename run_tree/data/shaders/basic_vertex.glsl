
layout (location = 0) in vec3 in_position;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in vec4 in_color;

out vec4 out_color;
out vec2 out_tex_coord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    out_color     = in_color;
    out_tex_coord = in_tex_coord;
    gl_Position = projection * view * model * vec4(in_position, 1);
}
