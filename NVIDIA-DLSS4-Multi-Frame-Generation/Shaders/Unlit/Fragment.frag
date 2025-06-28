#version 450

//layout (location = 0) in vec3 inColour;

layout (location = 0) out vec4 outColour;

layout(push_constant) uniform PushConstants
{
    mat2 transform;
    vec2 offset;
    vec3 colour;
} push;

void main()
{
    outColour = vec4(push.colour, 1.0);
    //outColour = vec4(inColour, 1.0);
}