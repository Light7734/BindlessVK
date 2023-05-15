#version 450 core
#pragma shader_stage(vertex)

#include "scene_descriptors.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 out_uvw;

void main()
{
    out_uvw = in_position;
    mat4 view = mat4(mat3(u_camera.view));
    vec4 pos = u_camera.proj * view * vec4(in_position.xyz, 1.0);

    gl_Position = pos.xyww;
}
