#version 460 core

layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;

// for uniform buffer
layout (binding = 0) uniform UniformBufferObject
{
    mat4 u_modelMatrix;
    mat4 u_viewMatrix;
    mat4 u_projectionMatrix;

    vec4 u_lightAmbient;
    vec4 u_lightDiffuse;
    vec4 u_lightSpecular;
    vec4 u_lightPosition;

    vec4 u_materialAmbient;
    vec4 u_materialDiffuse;
    vec4 u_materialSpecular;
    float u_materialShininess;

    bool u_lightEnabled;
} ubo;


layout(location=0) out vec3 o_transformedNormals;
layout(location=1) out vec3 o_lightDirection;
layout(location=2) out vec3 o_viewerVector;

void main()
{
    if (ubo.u_lightEnabled == true)
    {

        vec4 eyeCoordinates = ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(ubo.u_viewMatrix * ubo.u_modelMatrix)));
        o_transformedNormals = normalize(normalMatrix * a_normal);
        o_lightDirection = normalize(vec3(ubo.u_lightPosition - eyeCoordinates));

        o_viewerVector = normalize(-eyeCoordinates.xyz);

    }

    gl_Position = ubo.u_projectionMatrix * ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
}
