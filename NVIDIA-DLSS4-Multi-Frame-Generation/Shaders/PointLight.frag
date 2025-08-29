#version 450

layout(location = 0) in vec2 inFragOffset;

layout(location = 0) out vec4 outFragColor;
layout(location = 1) out vec2 outMotion;

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

const float M_PI = 3.14159265358;

void main()
{
    float dis = sqrt(dot(inFragOffset, inFragOffset));
    if (dis >= 1.0) {
        discard;
    }

    float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
    outFragColor = vec4(push.colour.xyz + cosDis, cosDis);
    outMotion = vec2(0.0, 0.0);
}