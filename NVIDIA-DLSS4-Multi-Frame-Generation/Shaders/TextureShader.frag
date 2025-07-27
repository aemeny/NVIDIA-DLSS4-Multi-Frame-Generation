#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 inFragPosWorld;
layout (location = 2) in vec3 inFragNormalWorld;
layout (location = 3) in vec2 inFragUv;

layout (location = 0) out vec4 outColour;

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

layout (set = 1, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
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
    outColour = vec4(diffuseLight * color + specularLight * inFragColor, 1.0);
}