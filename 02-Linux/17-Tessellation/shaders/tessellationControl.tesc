#version 460 core

layout(vertices=4) out;

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
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_TessLevelOuter[0] = float(ubo.u_numberOfStrips);
    gl_TessLevelOuter[1] = float(ubo.u_numberOfSegments);
}