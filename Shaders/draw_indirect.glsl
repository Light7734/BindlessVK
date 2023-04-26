#version 450 core
#pragma shader_stage(compute)

#extension GL_EXT_nonuniform_qualifier:enable

layout(local_size_x = 64) in;

struct ModelInformation
{
    mat4 model;

    float x;
    float y;
    float z;
    float r;

    int albedo_texture_index;
    int normal_texture_index;
    int metallic_roughness_texture_index;

    uint first_index;
    uint index_count;

    uint _pad0;
    uint _pad1;
    uint _pad2;
};

struct VkDrawIndexedIndirectCommand
{
    uint index_count;
    uint instance_count;
    uint first_index;
    uint vertex_offset;
    uint first_instance;
};

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    uint model_count;
} u_camera;

layout(set = 0, binding = 1) uniform Scene
{
    vec4 light_position;
    uint model_count;
} u_scene;

layout(std430, set = 0, binding = 0) buffer readonly ModelInformationBuffer
{
    ModelInformation arr[];
} b_models;

layout(std430, set = 0, binding = 3) buffer writeonly CommandBuffer
{
    VkDrawIndexedIndirectCommand arr[];
} b_commands;

bool is_visible(mat4 mat, vec3 origin, float radius)
{
    return true;
}

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if(id >= u_globals.model_count)
        return;

    ModelInformation model = b_models.arr[id];
    b_commands.arr[id].instance_count = is_visible(u_camera.proj_view, vec3(model.x, model.y, model.z), model.r) ? 1 : 0;
}
