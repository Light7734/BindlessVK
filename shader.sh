glslc -fshader-stage=vert ./Shaders/vertex.glsl -o ./Shaders/vertex.spv
glslc -fshader-stage=fragment ./Shaders/pixel.glsl -o ./Shaders/pixel.spv

glslc -fshader-stage=vert ./Shaders/skybox_vertex.glsl -o ./Shaders/skybox_vertex.spv
glslc -fshader-stage=fragment ./Shaders/skybox_fragment.glsl -o ./Shaders/skybox_fragment.spv
