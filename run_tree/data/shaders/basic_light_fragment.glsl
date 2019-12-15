
#version 300 es
precision highp float;

out vec4 fragment_color;

in vec3 out_position;
in vec3 out_normal;
in vec4 out_color;

struct Light {
    vec3 position;
};

const int MAX_LIGHTS = 8;
uniform Light lights[MAX_LIGHTS];
uniform int num_lights;

void main() {

    vec4 accum = vec4(0);
    for (int i = 0; i < 1; ++i) {
        vec4 light_color = vec4(1, 1, 1, 1);
        vec3 position = lights[i].position;

        vec3 P = normalize(position - out_position);
        vec3 N = normalize(out_normal);

        accum += dot(N, P) * light_color * out_color;
    }
    fragment_color = accum;
}


