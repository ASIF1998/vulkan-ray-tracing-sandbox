#ifndef TEXTURE_SAMPLING_GLSL
#define TEXTURE_SAMPLING_GLSL

vec4 uv_derives_from_ray_cone
(
	vec3    dir,
	vec3    world_normal,
	float   ray_cone_width,
	vec2    uv[3],
	vec3    pos[3],
	mat3    world_matrix
)
{
	vec2 uv10           = uv[1] - uv[0];
	vec2 uv20           = uv[2] - uv[0];
	float quad_uv_area  = abs(uv10.x * uv20.y + uv20.x * uv10.y);

	vec3 edge10         = world_matrix * (pos[1] - pos[0]);
	vec3 edge20         = world_matrix * (pos[2] - pos[0]);
	vec3 face_normal    = cross(edge10, edge20);
	float quad_area     = length(face_normal);

	float normal_term           = abs(dot(dir, world_normal));
	float projection_cone_width = ray_cone_width / normal_term;
	float visible_area_ratio    = (projection_cone_width * projection_cone_width) / quad_area;

	float visible_uv_area   = quad_uv_area * visible_area_ratio;
	float u_length          = sqrt(visible_uv_area);

	return vec4(u_length, 0, 0, u_length);
}

vec4 textureSampling(sampler2D image, surface_t surf, float eye_to_pixel_cone_spread_angle)
{
    vec3 hit_position       = vec3(gl_WorldRayDirectionEXT * gl_HitTEXT + gl_WorldRayOriginEXT);
    vec3 camera_position    = vec3(gl_WorldRayOriginEXT);

    vec2 ray_cone_at_origine = vec2(0, eye_to_pixel_cone_spread_angle);
    
    vec2 ray_cone_at_hit = vec2(
        ray_cone_at_origine.x + ray_cone_at_origine.y * length(hit_position - camera_position),
        ray_cone_at_origine.y + eye_to_pixel_cone_spread_angle
    );

    vec3    world_normal    = normalize(vec3(gl_ObjectToWorldEXT * vec4(surf.normal, 0)));
    float   ray_cone_width  = ray_cone_at_hit.x;
    mat3    world_matrix    = mat3(gl_ObjectToWorldEXT);

    vec4 uv_derives = uv_derives_from_ray_cone(
        gl_WorldRayDirectionEXT.xyz, 
        world_normal, 
        ray_cone_width, 
        surf.triangle.uvs, 
        surf.triangle.positions, 
        world_matrix
    );

    return textureGrad(image, surf.uv, uv_derives.xy, uv_derives.zw);
}

#endif