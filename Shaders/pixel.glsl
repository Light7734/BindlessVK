#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

#include "scene_descriptors.glsl"

layout(location = 0) in vec3 in_fragment_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_tangent_light_position;
layout(location = 3) in vec3 in_tangent_view_position;
layout(location = 4) in vec3 in_tangent_fragment_position;
layout(location = 5) in flat int in_instance_index;

layout(location = 0) out vec4 out_color;

void main()
{
    ModelData model_data = ub_models.arr[in_instance_index];
    int albedo_index = model_data.albedo_texture_index;
    int normal_index = model_data.normal_texture_index;

    vec3 normal = texture(u_textures[normal_index], in_uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 color = texture(u_textures[albedo_index], in_uv).rgb;

    vec3 ambient = 0.1 * color;

    vec3 light_dir = normalize(in_tangent_light_position - in_tangent_fragment_position);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * color;

    vec3 view_dir = normalize(in_tangent_view_position - in_tangent_fragment_position);
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;

    out_color = vec4(ambient + diffuse + specular, 1.0);
}
