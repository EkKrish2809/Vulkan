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


layout(location=0) out vec3 phong_ads_light;

void main()
{
    if (ubo.u_lightEnabled == true)
    {
        vec3 ambient = vec3(ubo.u_lightAmbient) * vec3(ubo.u_materialAmbient);

        vec4 eyeCoordinates = ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(ubo.u_viewMatrix * ubo.u_modelMatrix)));
        vec3 transformedNormals = normalize(normalMatrix * a_normal);
        vec3 lightDirection = normalize(vec3(ubo.u_lightPosition - eyeCoordinates));
        vec3 diffuse = vec3(ubo.u_lightDiffuse) * vec3(ubo.u_materialDiffuse) * max(dot(lightDirection, transformedNormals), 0.0);

        vec3 reflectionVector = reflect(-lightDirection, transformedNormals);
        vec3 viewerVector = normalize(-eyeCoordinates.xyz);
        vec3 specular = vec3(ubo.u_lightSpecular) * vec3(ubo.u_materialSpecular) * pow(max(dot(reflectionVector, viewerVector), 0.0), ubo.u_materialShininess);

        phong_ads_light = ambient + diffuse + specular;
    }
    else
    {
        phong_ads_light = vec3(1.0, 1.0, 1.0);
    }

    gl_Position = ubo.u_projectionMatrix * ubo.u_viewMatrix * ubo.u_modelMatrix * vec4(a_position, 1.0);
}
