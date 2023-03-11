#version 450 core
#pragma shader_stage(vertex)

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec3 in_color;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec3 out_view_vec;
layout(location = 4) out vec3 out_light_vec;
layout(location = 5) out vec3 out_fragment_position;
layout(location = 6) out flat int out_texture_index;
layout(location = 7) out mat3 out_tbn;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
} u_camera;

layout(set = 0, binding = 1) uniform Lights {
    vec4 light_position;
} u_lights;

void main() 
{
    mat4 view_proj = u_camera.projection * u_camera.view;
    gl_Position =  view_proj *  mat4(1.0) * vec4(in_position, 1.0);

    out_fragment_position = (mat4(1.0) * vec4(in_position, 1.0)).xyz;
    out_normal = in_normal;
    out_uv = in_uv;
    out_color = in_color;

    vec3 T = normalize(vec3(mat4(1.0) * vec4(in_tangent, 0.0)));
    vec3 N = normalize(vec3(mat4(1.0) * vec4(in_normal, 0.0)));
    vec3 B = cross(N, T);
    out_tbn = mat3(T, B, N);

    out_normal = mat3(u_camera.view) * in_normal;

    out_light_vec = u_lights.light_position.xyz;
    out_view_vec = u_camera.viewPos.xyz;

    out_texture_index = gl_InstanceIndex;
}

