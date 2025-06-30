#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 UV;

layout(location = 0) out vec3 outFragColour;
layout(location = 1) out vec3 outPosWorld;
layout(location = 2) out vec3 outFragNormalWorld;

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
    vec4 positionToWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionViewMatrix * positionToWorld;

    outFragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    outPosWorld = positionToWorld.xyz;
    outFragColour = colour;
}