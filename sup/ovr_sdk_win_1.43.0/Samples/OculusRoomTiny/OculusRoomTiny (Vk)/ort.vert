#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, push_constant) uniform buf
{
    mat4 mvp;
} ubuf;

layout (location = 0) in vec4 Position;
layout (location = 1) in vec4 Color;
layout (location = 2) in vec2 TexCoord;

out gl_PerVertex
{
    vec4 gl_Position;
};
layout (location = 0) out vec4 oColor;
layout (location = 1) out vec2 oTexCoord;

void main()
{
    // convert from sRGB to linear (really? vertex colors should be linear to start with...)
    oColor.rgb  = pow(Color.rgb, vec3(2.2));
    oColor.a = Color.a;
    oTexCoord = TexCoord;
    gl_Position = ubuf.mvp * Position;
}
