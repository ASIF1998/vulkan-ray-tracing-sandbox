#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable

#include <shaders/animation/ray_payload.glsl>

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT payload_t payload;

void main() 
{
    payload.col = vec3(0.43, 0.13, 0.54);
} 