#define max_point_light_count 32

struct Primitive
{
    float x;
    float y;
    float z;
    float radius;

    int albedo_index;
    int normal_index;
    int mr_index;
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
    uint _0;
    uint _1;
    uint _2;
};

struct DirectionalLight
{
    vec4 direction;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight
{
    vec4 position;

    float constant;
    float linear;
    float quadratic;
    float _0;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct Camera
{
    mat4 view;
    mat4 proj;
    mat4 view_proj;

    vec4 view_position;
};

struct Scene
{
    DirectionalLight directional_light;

    PointLight[max_point_light_count] point_lights;
    uint point_light_count;

    uint primitive_count;
};

layout(set = 0, binding = 0) uniform U_Frame
{
    Camera camera;
    Scene scene;

    float delta_time;
} u_frame;

layout(std430, set = 0, binding = 1) readonly buffer SSBO_Primitives
{
    Primitive arr[];
} ssbo_primitives;

layout(std430, set = 0, binding = 2) writeonly buffer SSBO_CommandBuffers
{
    IndirectCommand arr[];
} ssbo_indirect_commands;

layout(set = 0, binding = 3) uniform sampler2D s_textures[];
layout(set = 0, binding = 4) uniform samplerCube s_texture_cubes[];
