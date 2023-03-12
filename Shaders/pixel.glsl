#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 in_fragment_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_tangent_light_position;
layout(location = 3) in vec3 in_tangent_view_position;
layout(location = 4) in vec3 in_tangent_fragment_position;
layout(location = 5) in flat int in_instance_index;


layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 3) uniform sampler2D u_textures[];

struct ObjectData {
    int albedo_texture_index;
    int normal_texture_index;
    int metallic_roughness_texture_index;
    int _;
    mat4 model;
};

layout(set = 0, binding = 2) readonly buffer Objects{
    ObjectData data[];
} ub_objects;

void main()
{
    ObjectData object_data = ub_objects.data[in_instance_index];
    int albedo_index = object_data.albedo_texture_index;
    int normal_index = object_data.normal_texture_index;

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
