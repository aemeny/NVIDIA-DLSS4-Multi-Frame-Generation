#version 450

layout (location = 0) in vec3 inFragColour;

layout (location = 0) out vec4 outColour;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    vec3 colour;
} push;

void main()
{
    outColour = vec4(inFragColour, 1.0);
}