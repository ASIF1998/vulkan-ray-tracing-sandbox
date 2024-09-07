#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable
#extension GL_EXT_buffer_reference      : enable
#extension GL_EXT_scalar_block_layout   : enable

#include <shaders/animation/ray_payload.glsl>
#include <shaders/animation/shared.glsl>

#include <shaders/utils/scene_geometry.glsl>

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT payload_t payload;

layout(std430, set = 0, binding = scene_geometry_binding) buffer scene_geometry_b
{
    scene_vertices_t    scene_geometries;
    scene_indices_t     scene_indices;
} scene_info;

void main() 
{
    vec3 barycentric_coordinates = vec3(
        1.0 - attribs.x - attribs.y, attribs.x, attribs.y
    );

    surface_t surface = get_surface(
        scene_info.scene_geometries, 
        scene_info.scene_indices, 
        barycentric_coordinates
    );  

    payload.col = surface.normal * 0.5 + 0.5;
} 