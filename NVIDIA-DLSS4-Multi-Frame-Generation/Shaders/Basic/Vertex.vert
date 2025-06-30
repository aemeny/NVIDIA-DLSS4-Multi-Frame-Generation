#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 UV;

layout(location = 0) out vec3 outFragColour;
layout(location = 1) out vec3 outPosWorld;
layout(location = 2) out vec3 outFragNormalWorld;

struct PointLight
{
    vec4 position; // Ignore W
    vec4 colour; // W is intensity
};
layout(set = 0, binding = 0) uniform globalUbo{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    vec4 ambientLightColour;
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main()
{
    vec4 positionToWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionToWorld);

    outFragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    outPosWorld = positionToWorld.xyz;
    outFragColour = colour;
}