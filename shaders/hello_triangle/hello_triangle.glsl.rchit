#version 460    

#extension GL_EXT_ray_tracing: enable

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT vec3 hit_value;

void main()
{
    vec3 barycentric_coords = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    hit_value = barycentric_coords;
}