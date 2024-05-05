#ifndef CAMERA_GLSL
#define CAMERA_GLSL

struct camera_t
{
	mat4 inv_view_matrix;
	mat4 inv_projection_matrix;
};

#endif