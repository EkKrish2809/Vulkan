#version 460 core

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_color;
// layout(location=2) in vec2 a_texcoords;

// for uniform buffer
layout (binding = 0) uniform UniformBufferObject
{
    mat4 u_modelMatrix;
    mat4 u_viewMatrix;
    mat4 u_projectionMatrix;
} ubo;


layout(location=0) out vec3 o_color;
layout(location=1) out vec2 o_texcoords;

void main()
{
    gl_Position = ubo.u_projectionMatrix * ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
	o_color = a_color;
    // o_texcoords = a_texcoords;
}
