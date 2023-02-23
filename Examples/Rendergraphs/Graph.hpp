#pragma once

#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

class BasicRendergraph: public bvk::Rendergraph
{
public:
    struct FrameData
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec4 viewPos;

        static constexpr u32 binding = 0;
    };

    struct SceneData
    {
        glm::vec4 light_pos;
        
        static constexpr u32 binding = 1;
    };

public:
    BasicRendergraph(ref<bvk::VkContext> vk_context);

    virtual void on_update(u32 frame_index, u32 image_index) override;
};
