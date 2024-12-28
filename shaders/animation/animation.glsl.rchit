#version 460

#extension GL_EXT_ray_tracing           : enable
#extension GL_GOOGLE_include_directive	: enable
#extension GL_EXT_buffer_reference      : enable
#extension GL_EXT_scalar_block_layout   : enable
#extension GL_EXT_nonuniform_qualifier  : enable

#include <shaders/animation/ray_payload.glsl>
#include <shaders/animation/shared.glsl>

#include <shaders/utils/scene_geometry.glsl>
#include <shaders/utils/texture_sampling.glsl>

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT payload_t payload;

layout(std430, set = 0, binding = scene_geometry_binding) buffer scene_geometry_b
{
    scene_vertices_t    scene_geometries;
    scene_indices_t     scene_indices;
} scene_info;

layout(set = 0, binding = albedos_binding) uniform sampler2D albedos[];

layout(push_constant) uniform push_constants_t
{
	layout(offset = 128) float eye_to_pixel_cone_spread_angle;
} push_constants;

/// based on https://www.shadertoy.com/view/Xds3zN
vec3 render(vec3 albedo, vec3 normal)
{
    const vec3 lig = normalize(vec3(1));

    vec3  hal = reflect(gl_WorldRayDirectionEXT, lig);
    float dif = clamp(dot(normal, lig), 0.0, 1.0);

    float spe = pow(clamp(dot(normal, hal), 0.0, 1.0), 16.0);
    spe *= dif;
    spe *= 0.04 + 0.96 * pow(clamp(1.0 - dot(hal, lig), 0.0, 1.0), 5.0);

    vec3 col = albedo * 2.20 * dif * vec3(1.10, 1.00, 1.70);
    col += 1.00 * spe * vec3(1.10, 1.00, 1.70);
    col += albedo;

    return col * 0.5;
}

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

    vec3 albedo = textureSampling(
        albedos[nonuniformEXT(gl_InstanceCustomIndexEXT)], 
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).rgb;

    vec3 ref = reflect(gl_WorldRayOriginEXT, surface.normal);

    payload.col = render(albedo, surface.normal);
} 