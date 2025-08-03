#version 460 core

layout(location = 0) in vec3 phong_ads_light;

layout(location = 0)out vec4 FragColor;

void main()
{
    FragColor = vec4(phong_ads_light, 1.0);
}