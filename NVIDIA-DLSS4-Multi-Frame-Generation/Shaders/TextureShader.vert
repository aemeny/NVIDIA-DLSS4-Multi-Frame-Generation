#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 UV;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec3 outFragPosWorld;
layout(location = 2) out vec3 outFragNormalWorld;
layout(location = 3) out vec2 outFragUv;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;

void main() 
{
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projection * ubo.view * positionWorld;
    outFragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    outFragPosWorld = positionWorld.xyz;
    outFragColor = color;
    outFragUv = UV;
}