#version 460 core

layout(location=0) in vec3 o_color;
layout(location = 0)out vec4 FragColor;

void main()
{
    FragColor = vec4(o_color, 1.0);
}