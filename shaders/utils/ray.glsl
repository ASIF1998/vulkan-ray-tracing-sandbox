#ifndef RAY_GLSL
#define RAY_GLSL

#include <shaders/utils/camera.glsl>

struct ray_t 
{
	vec3 or;
	vec3 dir;
};
 
ray_t get_primary_ray(in camera_t camera, ivec2 offset_in_screen_space)
{
	const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy + vec2(offset_in_screen_space)) + vec2(0.5);
	const vec2 uv			= pixel_center / vec2(gl_LaunchSizeEXT.xy);

	vec2 d = uv * 2.0 - 1.0;

	vec4 original 	= camera.inv_view_matrix * vec4(0, 0, 0, 1);
	vec4 target 	= camera.inv_projection_matrix * vec4(d.x, d.y, 1, 1);
	vec4 direction 	= camera.inv_view_matrix * vec4(normalize(target.xyz), 0);

	return ray_t(
		original.xyz,
		direction.xyz
	);
}

#endif