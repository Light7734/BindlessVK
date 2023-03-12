#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 4) uniform samplerCube u_texture_cubes[];

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
    out_color = texture(u_texture_cubes[0],  in_uvw);
}
