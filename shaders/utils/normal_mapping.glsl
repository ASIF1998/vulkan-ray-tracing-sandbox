#ifndef NORMAL_MAPPING_GLSL
#define NORMAL_MAPPING_GLSL

vec3 get_shading_normal (
    vec3 normal_from_tangent_space,
    vec2 uv,
    vec3 normal,
    vec3 tangent
)
{
    vec3 shading_normal = normal_from_tangent_space * 2.0 - 1.0;
    
    mat3 tbn = mat3(
        tangent,
        cross(normal, tangent),
        normal
    );

    shading_normal = normalize(tbn * shading_normal);

    mat3 normal_matrix = transpose(inverse(mat3(gl_ObjectToWorldEXT)));

    return normalize(normal_matrix * shading_normal);
}

#endif