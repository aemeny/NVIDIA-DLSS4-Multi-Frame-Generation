#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 inFragPosWorld;
layout (location = 2) in vec3 inFragNormalWorld;
layout (location = 3) in vec2 inFragUv;
layout (location = 4) in vec4 inCurrClip;
layout (location = 5) in vec4 inPrevClip;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec2 outMotion; //R16G16_SFLOAT target

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
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

layout (set = 1, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform PushConstants 
{
  mat4 modelMatrix;
  mat4 normalMatrix;
  mat4 prevModel;
} push;

void main() 
{
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(inFragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - inFragPosWorld);

    for (int i = 0; i < ubo.numLights; i++) 
    {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - inFragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0);
        specularLight += intensity * blinnTerm;
    }

    vec3 color = texture(diffuseMap, inFragUv).xyz;

    vec3 lighting = diffuseLight * color + specularLight * inFragColor;
    lighting = pow(lighting, vec3(1.0 / 2.2)); // Gamma correction for UNORM swapchain output

    outColour = vec4(lighting, 1.0);

    // Motion Vectors
    vec2 cNDC = inCurrClip.xy / max(inCurrClip.w, 1e-6);
    vec2 pNDC = inPrevClip.xy / max(inPrevClip.w, 1e-6);
    vec2 ndcDelta = cNDC - pNDC;
    vec2 motionPx = 0.5 * ndcDelta * max(ubo.renderSize, vec2(1.0));
    outMotion = clamp(motionPx, vec2(-1e4), vec2(1e4));
}