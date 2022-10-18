glslc -fshader-stage=vert ./Shaders/vertex.glsl -o ./Shaders/defaultVertex.spv_vs
glslc -fshader-stage=fragment ./Shaders/pixel.glsl -o ./Shaders/defaultFragment.spv_fs

glslc -fshader-stage=vert ./Shaders/skybox_vertex.glsl -o ./Shaders/skybox_vertex.spv_vs
glslc -fshader-stage=fragment ./Shaders/skybox_fragment.glsl -o ./Shaders/skybox_fragment.spv_fs
