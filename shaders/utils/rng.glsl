#ifndef RNG_GLSL
#define RNG_GLSL

#include <shaders/utils/math_constants.glsl>

uint xorshift(uint x) 
{
    x ^= x << 13u;
    x ^= x >> 17u;
    x ^= x << 5u;
    return x;
}

uint hash(uint x)
{
    return xorshift(x);
}

uint hash(uvec2 v)
{
    return hash(v.x ^ hash(v.y));
}

uint hash(uvec3 v)
{
    return hash(v.x ^ hash(v.y) ^ hash(v.z));
}

uint hash(uvec4 v)
{
    return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w));
}

float uint_to_float(uint m)
{
    const uint ieee_mantissa    = 0x007fffffu;
    const uint ieee_one         = 0x3f800000u;

    m &= ieee_mantissa;
    m |= ieee_one;

    float f = uintBitsToFloat(m);

    return f - 1.0;
}

float random(float x)
{
    return uint_to_float(hash(floatBitsToUint(x)));
}

float random(vec2 v)
{
    return uint_to_float(hash(floatBitsToUint(v)));
}

float random(vec3 v)
{
    return uint_to_float(hash(floatBitsToUint(v)));
}

float random(vec4 v)
{
    return uint_to_float(hash(floatBitsToUint(v)));
}

vec4 random2(vec2 coord)
{
    vec2 uv = coord - (floor(coord / 289.0) * 289.0);
    uv += vec2(223.35734, 550.56781);
    uv *= uv;

    float xy = uv.x * uv.y;

    return vec4 (
        fract(xy * 0.00000012),
        fract(xy * 0.00000543),
        fract(xy * 0.00000192),
        fract(xy * 0.00000423)
    );
}

vec3 cosine_sample_hemisphere(vec3 n, inout vec2 seed)
{
    vec4 u = random2(seed);

    float r     = sqrt(u.x);
    float theta = 2.0 * PI * u.y;

    vec3 b = normalize(cross(n, vec3(0, 1, 1)));
    vec3 t = cross(b, n);

    return normalize(r * sin(theta) * b + sqrt(1.0 - u.x) * n + r * cos(theta) * t);
}

#endif