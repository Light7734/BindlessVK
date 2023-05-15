struct ModelData
{
    float x;
    float y;
    float z;
    float radius;

    int albedo_texture_index;
    int normal_texture_index;
    int metallic_roughness_texture_index;
    int _0;

    mat4 model;
};

struct IndirectCommand 
{
    uint index_count;
    uint instance_count;
    uint first_index;
    uint vertex_offset;
    uint first_instance;

    uint object_id;
    uint batch_id;
    uint pad;
};

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 proj;
    vec4 view_position;
} u_camera;

layout(set = 0, binding = 1) uniform Scene
{
    vec4 light_position;
    uint model_count;
} u_scene;

layout(std430, set = 0, binding = 2) readonly buffer ModelDataBuffer
{
    ModelData arr[];
} ub_models;

layout(std430, set = 0, binding = 3) writeonly buffer CommandBuffer
{
    IndirectCommand  arr[];
} ub_commands;

layout(set = 0, binding = 4) uniform sampler2D u_textures[];

layout(set = 0, binding = 5) uniform samplerCube u_texture_cubes[];
