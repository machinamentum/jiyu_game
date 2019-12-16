
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
    vec3 eyepos = vec3(0, 0, 0);
    vec3 E = normalize(eyepos - out_position);

    vec4 diffuse = vec4(0);
    vec4 specular = vec4(0);
    for (int i = 0; i < 1; ++i) {
        vec4 light_color = vec4(1, 1, 1, 1);
        vec3 light_position = lights[i].position;

        vec3 L = normalize(light_position - out_position);
        vec3 N = normalize(out_normal);

        vec3 H = normalize(E + L);

        diffuse += dot(N, L) * light_color * out_color;
        specular += pow(max(dot(H,N), 0.0), 32.0) * light_color;
    }
    fragment_color = diffuse + specular;
}


