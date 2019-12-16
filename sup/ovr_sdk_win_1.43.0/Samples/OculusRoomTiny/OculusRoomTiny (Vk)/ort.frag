#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D Texture0;

layout (location = 0) in vec4 oColor;
layout (location = 1) in vec2 oTexCoord;

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = oColor * texture(Texture0, oTexCoord);
}
