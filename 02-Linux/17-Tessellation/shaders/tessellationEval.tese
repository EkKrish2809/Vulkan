#version 460 core

layout(isolines) in;

// for uniform buffer
layout (binding = 0) uniform UniformBufferObject
{
    mat4 u_modelMatrix;
    mat4 u_viewMatrix;
    mat4 u_projectionMatrix;

    int u_numberOfSegments;
    int u_numberOfStrips;
} ubo;

void main()
{
    vec3 p0 = gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz;
	vec3 p3 = gl_in[3].gl_Position.xyz;
	float u = gl_TessCoord.x;
	vec3 p = p0 * (1 - u) * (1 - u) * (1 - u) + p1 * 3*u * (1 - u) * (1 - u) + p2 * 3*u*u * (1 - u) + p3 *u*u*u;
    gl_Position = ubo.u_projectionMatrix * ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(p, 1.0);
}