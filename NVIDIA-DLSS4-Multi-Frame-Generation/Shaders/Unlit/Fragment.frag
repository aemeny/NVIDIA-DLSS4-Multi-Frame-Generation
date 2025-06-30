#version 450

layout (location = 0) in vec3 inFragColour;
layout (location = 1) in vec3 inPosWorld;
layout (location = 2) in vec3 inFragNormalWorld;

layout (location = 0) out vec4 outColour;

layout(set = 0, binding = 0) uniform globalUbo{
    mat4 projectionViewMatrix;
    vec4 ambientLightColour;
    vec3 lightPosition;
    vec4 lightColour;
} ubo;

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main()
{
    // Lighting
    vec3 directionToLight = ubo.lightPosition - inPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // Distance squared

    vec3 lightColour = ubo.lightColour.xyz * ubo.lightColour.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColour.xyz * ubo.ambientLightColour.w;
    vec3 diffuseLight = lightColour * max(dot(normalize(inFragNormalWorld), normalize(directionToLight)), 0.0);

    outColour = vec4((diffuseLight + ambientLight) * inFragColour, 1.0);
}