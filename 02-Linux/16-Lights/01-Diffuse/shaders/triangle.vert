#version 460 core

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;

// for uniform buffer
layout (binding = 0) uniform UniformBufferObject
{
    mat4 u_modelMatrix;
    mat4 u_viewMatrix;
    mat4 u_projectionMatrix;

    vec4 u_lightDiffuse;
    vec4 u_materialDiffuse;
    vec4 u_lightPosition;

    bool u_lightEnabled;
} ubo;


layout(location=0) out vec3 diffuse_light_color;

void main()
{
    if (ubo.u_lightEnabled == true)
    {
        vec4 eyeCoordinates = ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(ubo.u_viewMatrix * ubo.u_modelMatrix)));
        vec3 transformedNormals = normalize(normalMatrix * a_normal);
        vec3 lightDirection = normalize(vec3(ubo.u_lightPosition - eyeCoordinates));
        diffuse_light_color = vec3(ubo.u_lightDiffuse) * vec3(ubo.u_materialDiffuse) * max(dot(lightDirection, transformedNormals), 0.0);
    }
    else
    {
        diffuse_light_color = vec3(1.0, 1.0, 1.0);
    }

    gl_Position = ubo.u_projectionMatrix * ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
}
