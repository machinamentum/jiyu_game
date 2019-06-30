

#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;

out vec4 out_color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    out_color = vec4(1, 0, 0, 1);
    gl_Position = projection * view * model * vec4(in_position, 1);
}
