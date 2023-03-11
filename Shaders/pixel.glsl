#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec3 in_view_vec;
layout(location = 4) in vec3 in_light_vec;
layout(location = 5) in vec3 in_fragment_position;
layout(location = 6) in flat int in_texture_index;
layout(location = 7) in mat3 in_tbn;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform sampler2D u_textures[];

void main()
{
    vec4 color = texture(u_textures[in_texture_index], in_uv) * vec4(in_color, 1.0);

    vec3 N = texture(u_textures[in_texture_index - 2], in_uv).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(in_tbn * N);

    vec3 L = normalize(in_light_vec - in_fragment_position);
    vec3 V = normalize(in_view_vec - in_fragment_position);
    vec3 R = reflect(-L, N);

    vec3 ambient = color.rgb * 0.1;
    vec3 diffuse = max(dot(L, N), 0.0) * vec3(1.0, 1.0, 1.0);
    vec3 specular = pow(max(dot(V, R), 0.0), 32.0) * vec3(0.75);
    
    out_color = vec4((ambient + diffuse + specular) * color.rgb, 1.0);
}
