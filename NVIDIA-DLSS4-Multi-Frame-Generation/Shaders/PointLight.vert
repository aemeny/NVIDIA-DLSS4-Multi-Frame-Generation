#version 450

// Billboard obj offsets
const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) out vec2 outFragOffset;

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
    vec4 position;
    vec4 colour;
    float radius;
} push;

void main()
{
    outFragOffset = OFFSETS[gl_VertexIndex];

    vec4 lightInCameraSpace = ubo.viewMatrix * vec4(push.position.xyz, 1.0);
    vec4 positionInCameraSpace = lightInCameraSpace + push.radius * vec4(outFragOffset, 0.0, 0.0);
    gl_Position = ubo.projectionMatrix * positionInCameraSpace;
}