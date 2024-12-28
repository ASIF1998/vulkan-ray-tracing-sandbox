#ifndef GEOMETRY_PROPERTIES_GLSL
#define GEOMETRY_PROPERTIES_GLSL

struct attribute_t
{
    vec4 pos;
    vec4 normal;
    vec4 tangent;
    vec4 uv;
};

struct skinning_data_t
{
    ivec4   bone_ids;
    vec4    weights;
};

struct triangle_t
{
    vec3 positions[3];
    vec2 uvs[3];
};

struct surface_t
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;

    triangle_t triangle;
};

#endif