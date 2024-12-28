#version 460

#extension GL_EXT_ray_tracing                               : enable
#extension GL_EXT_buffer_reference                          : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64    : enable
#extension GL_EXT_scalar_block_layout                       : enable
#extension GL_EXT_nonuniform_qualifier                      : enable
#extension GL_GOOGLE_include_directive	                    : enable

#include <shaders/utils/scene_geometry.glsl>
#include <shaders/utils/normal_mapping.glsl>
#include <shaders/utils/texture_sampling.glsl>
#include <shaders/utils/math_constants.glsl>
#include <shaders/utils/rng.glsl>

#include <shaders/junk_shop/shared.glsl>
#include <shaders/junk_shop/ray_payload.glsl>
#include <shaders/junk_shop/material.glsl>
#include <shaders/junk_shop/pbr.glsl>

hitAttributeEXT vec2 attribs;

layout(set = 0, binding = acceleration_structure_binding) uniform accelerationStructureEXT scene;

layout(location = 0) rayPayloadInEXT payload_t payload;

layout(push_constant) uniform push_constants_t
{
	layout(offset = 132) float eye_to_pixel_cone_spread_angle;
} push_constants;

layout(std430, set = 0, binding = scene_geometry_binding) buffer scene_geometry_b
{
    scene_vertices_t    scene_geometries;
    scene_indices_t     scene_indices;
} scene_info;

layout(set = 0, binding = albedos_binding)      uniform sampler2D albedos[];
layout(set = 0, binding = normal_maps_binding)  uniform sampler2D normal_maps[];
layout(set = 0, binding = metallic_binding)     uniform sampler2D metallic[];
layout(set = 0, binding = roughness_binding)    uniform sampler2D roughness[];
layout(set = 0, binding = emissives_binding)    uniform sampler2D emissives[];

material_t get_material(in surface_t surface)
{
    vec3 albedo = textureSampling(
        albedos[nonuniformEXT(gl_InstanceCustomIndexEXT)], 
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).rgb;

    vec3 emissive = textureSampling(
        emissives[nonuniformEXT(gl_InstanceCustomIndexEXT)], 
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).rgb;

    float metallic = textureSampling(
        metallic[nonuniformEXT(gl_InstanceCustomIndexEXT)],
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).r;

    float roughness = textureSampling(
        roughness[nonuniformEXT(gl_InstanceCustomIndexEXT)], 
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).r;

    vec3 normal_from_tangent_space = textureSampling(
        normal_maps[nonuniformEXT(gl_InstanceCustomIndexEXT)],
        surface, 
        push_constants.eye_to_pixel_cone_spread_angle
    ).xyz;

    vec3 normal = get_shading_normal(
        normal_from_tangent_space,
        surface.uv,
        surface.normal,
        surface.tangent
    );

    return material_t(albedo, emissive, normal, metallic, roughness);
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

    material_t material = get_material(surface);

    vec3 out_dir = vec3(0);

    vec3 v = -payload.ray.dir;

    brdf_t brdf     = sampleDisnayBRDF(payload.seed, v, material, out_dir);
    payload.seed    = vec2(dot(payload.seed, surface.pos.xy), dot(surface.normal.xy, payload.seed.yx));

    vec3 emissive = material.emissive;

    payload.acc += emissive * payload.abso;

    if (brdf.pdf > EPS)
        payload.abso *= brdf.result / brdf.pdf;

    payload.ray.or  = surface.pos + surface.normal * EPS;
    payload.ray.dir = out_dir;
}