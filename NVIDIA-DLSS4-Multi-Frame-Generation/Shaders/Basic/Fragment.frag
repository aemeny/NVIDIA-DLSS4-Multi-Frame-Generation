#version 450

layout (location = 0) in vec3 inFragColour;
layout (location = 1) in vec3 inPosWorld;
layout (location = 2) in vec3 inFragNormalWorld;

layout (location = 0) out vec4 outColour;

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
    // Lighting
    vec3 diffuseLight = ubo.ambientLightColour.xyz * ubo.ambientLightColour.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(inFragNormalWorld);

    vec3 cameraposWorld = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(cameraposWorld - inPosWorld);

    for (int i = 0; i < ubo.numLights; i++)
    {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - inPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight); // Distance squared
        directionToLight = normalize(directionToLight);

        float cosAngIncidence =  max(dot(normalize(inFragNormalWorld), directionToLight), 0.0);
        vec3 intensity = light.colour.xyz * light.colour.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        // Specular light calculation
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0.0, 1.0);
        blinnTerm = pow(blinnTerm, 512.0); // Shininess factor (Higher -> sharper highlight)
        specularLight += intensity * blinnTerm;
    }

    vec3 lighting = (diffuseLight * inFragColour) + (specularLight * inFragColour);
    lighting = pow(lighting, vec3(1.0 / 2.2)); // Gamma correction for UNORM swapchain output

    outColour = vec4(lighting, 1.0);
}