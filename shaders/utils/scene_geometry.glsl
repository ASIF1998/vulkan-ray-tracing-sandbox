#ifndef SCENE_GEOMETRY_GLSL
#define SCENE_GEOMETRY_GLSL

/// Vertices
struct attribute_t
{
    vec4 pos;
    vec4 normal;
    vec4 tangent;
    vec4 uv;
};

layout(std430, scalar, buffer_reference, buffer_reference_align = 16) readonly buffer vertex_buffer_t
{
    attribute_t attributes[];
};

layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer scene_vertices_t
{
    vertex_buffer_t vertex_buffers[];
};

/// Indices
layout(std430, scalar, buffer_reference, buffer_reference_align = 16) readonly buffer index_buffer_t
{
    uint indices[];
};

layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer scene_indices_t
{
    index_buffer_t index_buffers[];
};

struct triangle_t
{
    vec3 positions[3];
    vec2 uvs[3];
};

/// Helpers
struct surface_t
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;

    triangle_t triangle;
};

vec3 interpolate_attributes(vec3 v1, vec3 v2, vec3 v3, vec3 bc)
{
    return v1 * bc.x + v2 * bc.y + v3 * bc.z;
}

vec2 interpolate_attributes(vec2 v1, vec2 v2, vec2 v3, vec3 bc)
{
    return v1 * bc.x + v2 * bc.y + v3 * bc.z;
}

surface_t get_surface (
    in scene_vertices_t scene_geometries, 
    in scene_indices_t  scene_indices,
    vec3                barycentric_coordinates
) 
{
    mat3 normal_matrix = inverse(transpose(mat3(gl_ObjectToWorldEXT)));

    uint index_1 = scene_indices.index_buffers[gl_InstanceCustomIndexEXT].indices[gl_PrimitiveID * 3 + 0];
    uint index_2 = scene_indices.index_buffers[gl_InstanceCustomIndexEXT].indices[gl_PrimitiveID * 3 + 1];
    uint index_3 = scene_indices.index_buffers[gl_InstanceCustomIndexEXT].indices[gl_PrimitiveID * 3 + 2];

    vec3 pos_1 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_1].pos.xyz;
    vec3 pos_2 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_2].pos.xyz;
    vec3 pos_3 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_3].pos.xyz;

    vec3 normal_1 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_1].normal.xyz;
    vec3 normal_2 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_2].normal.xyz;
    vec3 normal_3 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_3].normal.xyz;

    vec3 tangent_1 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_1].tangent.xyz;
    vec3 tangent_2 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_2].tangent.xyz;
    vec3 tangent_3 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_3].tangent.xyz;

    vec2 uv_1 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_1].uv.xy;
    vec2 uv_2 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_2].uv.xy;
    vec2 uv_3 = scene_geometries.vertex_buffers[gl_InstanceCustomIndexEXT].attributes[index_3].uv.xy;

    triangle_t triangle;
    triangle.positions  = vec3[] (pos_1, pos_2, pos_3);
    triangle.uvs        = vec2[] (uv_1, uv_2, uv_3);

    vec3 normal = interpolate_attributes(normal_1, normal_2, normal_3, barycentric_coordinates);

    vec3 position = interpolate_attributes(pos_1, pos_2, pos_3, barycentric_coordinates);
    position = gl_ObjectToWorldEXT * vec4(position, 1);

    return surface_t (
        position,
        normalize(normal),
        interpolate_attributes(tangent_1, tangent_2, tangent_3, barycentric_coordinates),
        interpolate_attributes(uv_1, uv_2, uv_3, barycentric_coordinates),
        triangle
    );
}

#endif