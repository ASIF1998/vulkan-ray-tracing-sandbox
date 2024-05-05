#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable

#include <shaders/junk_shop/ray_payload.glsl>

layout(location = 0) rayPayloadInEXT payload_t payload;

vec3 sky_color(vec2 p, float f)
{
    float h = max(0.0, f - p.y - pow(abs(p.x - 0.5), 3.0));
    
    vec3 l =  vec3(pow(h, 3.0), pow(h, 7.0), 0.2 + pow(max(0.0, h - 0.1), 10.0)) * 1.5;

    const vec3 c1 = vec3(0.05);
    const vec3 c2 = vec3(51) / 255.0;

    return mix(c1, c2, l);
}

void main()
{
    payload.acc += sky_color(gl_WorldRayDirectionEXT.yz, 0.2) * payload.abso;

    payload.is_missed = true;
}