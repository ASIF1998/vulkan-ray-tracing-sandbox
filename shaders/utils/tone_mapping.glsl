#ifndef TONE_MAPPING_GLSL
#define TONE_MAPPING_GLSL

#include <shaders/utils/math_constants.glsl>

vec3 aces_film(vec3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0 , 1.0);
}

vec3 white_preserving_luma_based_reinhard(vec3 color)
{
    const float gamma = 2.2;

	float white = 20.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / max(luma, EPS);
	color = pow(color, vec3(1. / gamma));
	return color;
}

#endif