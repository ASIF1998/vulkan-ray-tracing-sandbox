#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable

#include <shaders/animation/ray_payload.glsl>

layout(location = 0) rayPayloadInEXT payload_t payload;

void main()
{
    payload.col = vec3(0.54, 0.97, 0.34);    
}