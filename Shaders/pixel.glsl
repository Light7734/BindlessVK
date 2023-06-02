#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

#include "global_descriptors.glsl"

layout(location = 0) in vec3 in_fragment_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_tangent_light_position;
layout(location = 3) in vec3 in_tangent_view_position;
layout(location = 4) in vec3 in_tangent_fragment_position;
layout(location = 5) in flat int in_instance_index;

layout(location = 0) out vec4 out_color;

vec3 calc_directional_light(DirectionalLight light, vec3 albedo, vec3 normal, vec3 view_dir, int albedo_index);
vec3 calc_point_light(PointLight light, vec3 albedo, vec3 normal, vec3 view_dir, int albedo_index);

void main()
{
    Primitive primitive = ssbo_primitives.arr[in_instance_index];

    int albedo_index = primitive.albedo_index;
    int normal_index = primitive.normal_index;

    vec3 normal = normalize(texture(s_textures[normal_index], in_uv).rgb * 2.0 - 1.0);
    vec3 albedo = texture(s_textures[albedo_index], in_uv).rgb;

    vec3 view_dir = normalize(in_tangent_view_position - in_tangent_fragment_position);

    vec3 result = calc_directional_light(u_frame.scene.directional_light, albedo, normal, view_dir, albedo_index);

    for(uint i = 0; i < u_frame.scene.point_light_count; ++i)
        result += calc_point_light(u_frame.scene.point_lights[i], albedo, normal, view_dir, albedo_index);

    out_color = vec4(result , 1.0);
}

vec3 calc_directional_light(DirectionalLight light, vec3 albedo, vec3 normal, vec3 view_dir, int albedo_index)
{
    vec3 light_dir = normalize(vec3(-light.direction));
    float diff = max(dot(normal, light_dir), 0.0);

    vec3 ambient = vec3(light.ambient) * albedo;
    vec3 diffuse = vec3(light.diffuse) * diff * albedo;

    return ambient + diffuse;
}

vec3 calc_point_light(PointLight light, vec3 albedo, vec3 normal, vec3 view_dir, int albedo_index)
{
    vec3 light_dir = normalize(vec3(light.position) - in_fragment_position);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);

    // attenuation
    float distance    = length(vec3(light.position) - in_fragment_position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    

    // combine results
    vec3 ambient  = vec3(light.ambient)  * albedo;
    vec3 diffuse  = vec3(light.diffuse)  * diff * albedo;

    ambient  *= attenuation;
    diffuse  *= attenuation;

    return ambient + diffuse;
}
