#version 460
#pragma shader_stage(compute)

#include "scene_descriptors.glsl"

layout(local_size_x = 64) in;

bool is_visible(mat4 mat, vec3 origin, float radius)
{
    uint plane_index = 0;
    for (uint i = 0; i < 3; ++i)
    {
        for (uint j = 0; j < 2; ++j, ++plane_index)
        {
            if (plane_index == 2 || plane_index == 3)
            {
                continue;
            }
            const float sign  = (j > 0) ? 1.f : -1.f;
            vec4 plane = vec4(0, 0, 0, 0);
            for (uint k = 0; k < 4; ++k)
            {
                plane[k] = mat[k][3] + sign * mat[k][i];
            }
            plane.xyzw /= sqrt(dot(plane.xyz, plane.xyz));
            if (dot(origin, plane.xyz) + plane.w + radius < 0)
            {
                return false;
            }
        }
    }
    return true;
}

void main()
{
    const uint id = gl_GlobalInvocationID.x;

    if (id > u_scene.model_count) 
        return;

    const ModelData model = ub_models.arr[id];
    const vec3 model_center = vec3(model.x, model.y, model.z);

    ub_commands.arr[id].instance_count =
        is_visible(u_camera.proj * u_camera.view, model_center, model.radius) ? 1 : 0;
}
