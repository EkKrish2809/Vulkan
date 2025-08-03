#version 460 core

layout(location = 0) in vec3 o_transformedNormals;
layout(location = 1) in vec3 o_lightDirection;
layout(location = 2) in vec3 o_viewerVector;

// uniform buffer
layout(binding = 0) uniform UniformBufferObject
{
	mat4 u_modelMatrix;
	mat4 u_viewMatrix;
	mat4 u_projectionMatrix;

	vec4 u_lightAmbient;
	vec4 u_lightDiffuse;
	vec4 u_lightSpecular;
	vec4 u_lightPosition;

	vec4 u_matrialAmbient;
	vec4 u_materialDiffuse;
	vec4 u_materialSpecular;
	float u_materialShininess;

	bool u_lightEnabled;
} ubo;

layout(location = 0)out vec4 FragColor;

void main()
{
	vec3 phongAdsLight;
	if (ubo.u_lightEnabled == true)
	{
		vec3 ambient = vec3(ubo.u_lightAmbient) * vec3(ubo.u_matrialAmbient);
		
		vec3 normalizedTransformedNormals = normalize(o_transformedNormals);
		vec3 normalizedLightDirection = normalize(o_lightDirection);
		vec3 diffuse = vec3(ubo.u_lightDiffuse) * vec3(ubo.u_materialDiffuse) * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0);
		
		vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals);
		vec3 normalizedViewerVector = normalize(o_viewerVector);
		vec3 specular = vec3(ubo.u_lightSpecular) * vec3(ubo.u_materialSpecular) * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0), ubo.u_materialShininess);

		phongAdsLight = ambient + diffuse + specular;
	}
	else
	{
		phongAdsLight = vec3(1.0, 1.0, 1.0);
	}
   	FragColor = vec4(phongAdsLight, 1.0);
}
