#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 colour;

//layout(location = 0) out vec3 outFragColour;

layout(push_constant) uniform PushConstants
{
    mat2 transform;
    vec2 offset;
    vec3 colour;
} push;

void main()
{
    gl_Position = vec4((push.transform * position) + push.offset, 0.0, 1.0);
    //outFragColour = colour;
}