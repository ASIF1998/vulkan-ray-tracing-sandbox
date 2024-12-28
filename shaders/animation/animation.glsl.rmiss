#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable

#include <shaders/animation/ray_payload.glsl>

layout(location = 0) rayPayloadInEXT payload_t payload;

void main()
{
    vec3 col = vec3(0.3, 0.5, 0.85) - gl_WorldRayDirectionEXT.y * gl_WorldRayDirectionEXT.y * 0.1;
    col = mix(col, 0.85 * vec3(0.7, 0.75, 0.85), pow(1.0 - max(gl_WorldRayDirectionEXT.y, 0.0), 4.0));
    payload.col = col;
}