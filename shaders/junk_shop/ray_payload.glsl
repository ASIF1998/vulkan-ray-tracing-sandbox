#ifndef RAY_PAYLOAD_GLSL
#define RAY_PAYLOAD_GLSL

#include <shaders/utils/ray.glsl>

struct payload_t
{
    vec2    seed;
    int     all_bounds;
    vec3    acc;
    vec3    abso;
    ray_t   ray;
    bool    is_missed;
};

#endif