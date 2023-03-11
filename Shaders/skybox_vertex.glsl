#version 450 core
#pragma shader_stage(vertex)

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec3 in_color;

layout(location = 0) out vec3 out_uvw;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
    vec4 view_position;
} u_camera;


layout(set = 0, binding = 1) uniform Lights{
    vec4 light_position;
} u_lights;

void main()
{
    out_uvw = in_position;
    mat4 view = mat4(mat3(u_camera.view));
    vec4 pos = u_camera.projection * view * vec4(in_position.xyz, 1.0);

    gl_Position = pos.xyww;
}
