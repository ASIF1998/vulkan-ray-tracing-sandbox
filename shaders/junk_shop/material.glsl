#ifndef JUNK_SHOP_MATERIAL_GLSL
#define JUNK_SHOP_MATERIAL_GLSL

struct material_t   
{
    vec3    albedo;
    vec3    emissive;
    vec3    shading_normal;
    float   metallic;
    float   roughness;
};

#endif