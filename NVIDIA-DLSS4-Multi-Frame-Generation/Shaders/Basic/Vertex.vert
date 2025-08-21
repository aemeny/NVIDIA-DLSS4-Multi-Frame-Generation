#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 UV;

layout(location = 0) out vec3 outFragColour;
layout(location = 1) out vec3 outPosWorld;
layout(location = 2) out vec3 outFragNormalWorld;
layout(location = 3) out vec4 outCurrClip;
layout(location = 4) out vec4 outPrevClip;

struct PointLight
{
    vec4 position; // Ignore W
    vec4 colour; // W is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo 
{
  mat4 projection;
  mat4 view;
  mat4 prevView;
  mat4 prevProjection;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  vec2 renderSize;
  int numLights;
} ubo;

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    mat4 normalMatrix;
    mat4 prevModel;
} push;

void main()
{
    vec4 positionToWorld = push.modelMatrix * vec4(position, 1.0);
    vec4 prevPositionToWorld = push.prevModel * vec4(position, 1.0);
    outCurrClip = ubo.projection * (ubo.view * positionToWorld);
    outPrevClip = ubo.prevProjection * (ubo.prevView * prevPositionToWorld);

    gl_Position = outCurrClip;

    outFragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    outPosWorld = positionToWorld.xyz;
    outFragColour = colour;
}