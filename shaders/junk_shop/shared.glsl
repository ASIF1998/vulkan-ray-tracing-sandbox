#ifndef JUNK_SHOP_BINDINGS_GLSL
#define JUNK_SHOP_BINDINGS_GLSL

#include <shaders/utils/ray.glsl>

const uint acceleration_structure_binding = 0u;

const uint result_image_binding         = 1u;
const uint accumulated_buffer_binding   = 2u;

const uint scene_geometry_binding = 3u;

const uint albedos_binding      = 4u;
const uint normal_maps_binding  = 5u;
const uint metallic_binding    	= 6u;
const uint roughness_binding    = 7u;
const uint emissives_binding    = 8u;

const int max_recursive = 7;

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