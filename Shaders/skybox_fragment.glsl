#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 5) uniform samplerCube u_texture_cubes[];

struct ModelData
{
    float x;
    float y;
    float z;
    float r;

    int albedo_texture_index;
    int normal_texture_index;
    int metallic_roughness_texture_index;
    int _pad;
    
    mat4 model;
};

layout(std430, set = 0, binding = 2) buffer readonly ModelDataBuffer
{
    ModelData arr[];
} ub_models;

void main()
{
    out_color = texture(u_texture_cubes[0],  in_uvw);
}
