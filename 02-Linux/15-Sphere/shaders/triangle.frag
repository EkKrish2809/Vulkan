#version 460 core

layout(location = 0) in vec3 o_color;
// layout(location = 1) in vec2 o_texcoords;

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0)out vec4 FragColor;

void main()
{
    // FragColor = texture(textureSampler, o_texcoords);// * vec4(o_color, 1.0);
    FragColor = vec4(o_color, 1.0);
}