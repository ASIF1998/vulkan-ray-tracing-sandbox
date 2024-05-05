#ifndef PBR_GLSL
#define PBR_GLSL

struct brdf_t
{
    vec3    result;
    float   pdf;
};

float luma(vec3 color) 
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 F_Schlick(vec3 f0, float theta)
{
    return f0 + (1.0 - f0) * pow(1.0 - theta, 5.0);
}

float F_Schlick(float f0, float f90, float theta)
{
    return f0 + (f90 - f0) * pow(1.0 - theta, 5.0);
}

float D_GTR(float roughness, float n_dot_h, float k)
{
    float a2 = roughness * roughness;
    return a2 / (PI * pow((n_dot_h * n_dot_h) * (a2 * a2 - 1.0) + 1.0, k));
}

float SmithG(float n_dot_v, float alpha_g)
{
    float a = alpha_g * alpha_g;
    float b = n_dot_v * n_dot_v;
    return (2.0 * n_dot_v) / (n_dot_v + sqrt(a + b - a * b));
}

float geometryTerm(float n_dot_l, float n_dot_v, float roughness)
{
    float a2 = roughness * roughness;
    return SmithG(n_dot_l, a2) * SmithG(n_dot_v, a2);
}

float GGXVNDPdf(float n_dot_h, float n_dot_v, float roughness)
{
    float D     = D_GTR(roughness, n_dot_h, 2.0);
    float G1    = SmithG(n_dot_v, roughness * roughness);
    return (D * G1) / max(EPS, 4.0 * n_dot_v);
}

vec3 sampleGGVNDF(vec3 v, float ax, float ay, float r1, float r2)
{
    vec3 vh = normalize(vec3(ax * v.x, ay * v.y, v.z));

    float   lensq   = vh.x * vh.x + vh.y * vh.y;
    vec3    t1      = lensq > 0.0 ? vec3(-vh.y, vh.x, 0) * inversesqrt(lensq) : vec3(1, 0, 0);
    vec3    t2      = cross(vh, t1);

    float r     = sqrt(r1);
    float phi   = 2.0 * PI * r2;
    float b1    = r * cos(phi);
    float b2    = r * sin(phi);
    float s     = 0.5 * (1.0 + vh.z);

    b2 = (1.0 - s) * sqrt(1.0 - b1 * b1) + s * b2;

    vec3 nh = b1 * t1 + b2 * t2 + sqrt(max(0.0, 1.0 - b1 * b1 - b2 * b2)) * vh;

    return normalize(vec3(ax * nh.x, ay * nh.y, max(0.0, nh.z)));
}

brdf_t sampleDisnayBRDF(vec2 seed, vec3 v, material_t material, out vec3 out_dir)
{
    float roughness = material.roughness * material.roughness;

    vec3 h = sampleGGVNDF(v, roughness, roughness, random(seed.x), random(seed.y));

    float v_dot_h = max(dot(v, h), EPS);

    vec3 f0 = mix(vec3(0.04), material.albedo, material.metallic);
    vec3 F  = F_Schlick(f0, v_dot_h);

    float diff_w    = (1.0 - material.metallic);
    float spec_w    = luma(F);
    float inv_w     = 1.0 / (diff_w + spec_w);

    diff_w *= inv_w;
    spec_w *= inv_w;

    if (random(seed) < diff_w)
    {
        out_dir = cosine_sample_hemisphere(material.shading_normal, seed);
        h       = normalize(out_dir + v);

        float n_dot_l = max(dot(material.shading_normal, out_dir), EPS);
        float n_dot_v = max(dot(material.shading_normal, v), EPS);

        if (n_dot_l > EPS)
        {
            float l_dot_h = max(dot(out_dir, h), EPS);

            float f90   = 0.5 + 2.0 * roughness * pow(l_dot_h, 2.0);
            float a     = F_Schlick(1.0, f90, n_dot_l);
            float b     = F_Schlick(1.0, f90, n_dot_v);

            vec3 diff = material.albedo * (a * b / PI);

            return brdf_t(diff * n_dot_l, diff_w * (n_dot_l / PI));
        }
    }
    else
    {
        out_dir = reflect(-v, h);

        float n_dot_l = max(dot(material.shading_normal, out_dir), EPS);
        float n_dot_v = max(dot(material.shading_normal, v), EPS);

        if (n_dot_l > EPS && n_dot_v > EPS)
        {
            float n_dot_h = max(dot(material.shading_normal, h), EPS);

            float   D = D_GTR(roughness, n_dot_h, 2.0);
            float   G = geometryTerm(n_dot_l, n_dot_v, pow(0.5 + material.roughness * 0.5, 2.0));

            vec3 spec = (F * G * D) / (4.0 * n_dot_l * n_dot_v);

            return brdf_t(spec * n_dot_l, spec_w * GGXVNDPdf(n_dot_h, n_dot_v, roughness));
        }
    }

    return brdf_t(vec3(0), EPS);
}

#endif