#ifndef RAY_PAYLOAD_GLSL
#define RAY_PAYLOAD_GLSL

#include <shaders/utils/ray.glsl>

struct payload_t
{
    vec3    col;
    ray_t   ray;
};

#endif