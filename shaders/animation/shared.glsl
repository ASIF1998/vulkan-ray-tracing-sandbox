#ifndef ANIMATION_SHARED_GLSL
#define ANIMATION_SHARED_GLSL

const uint result_image_binding             = 0;
const uint acceleration_structure_binding   = 1;
const uint scene_geometry_binding			= 2;

float infinity = uintBitsToFloat(0x7F800000);

#define trace(ray, scene)           \
    do {                            \
        traceRayEXT (               \
	    	scene,					\
	    	gl_RayFlagsOpaqueEXT,	\
	    	0xff,					\
	    	0, 0, 					\
	    	0,						\
	    	ray.or,					\
	    	0.2,					\
	    	ray.dir,				\
	    	infinity, 				\
	    	0						\
	    );              			\
    } while (false)

#endif