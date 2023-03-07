glslc --target-env=vulkan1.2 ./Shaders/vertex.glsl -o ./Shaders/vertex.spv
glslc --target-env=vulkan1.2 ./Shaders/pixel.glsl  -o ./Shaders/pixel.spv

glslc --target-env=vulkan1.2 ./Shaders/skybox_vertex.glsl   -o ./Shaders/skybox_vertex.spv
glslc --target-env=vulkan1.2 ./Shaders/skybox_fragment.glsl -o ./Shaders/skybox_fragment.spv
