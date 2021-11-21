#pragma once

#include "Core/Base.h"
#include "Graphics/RendererPrograms/RendererProgram.h"

#include <volk.h>

#define MAX_QUAD_RENDERER_VERTICES 1000u * 4u
#define MAX_QUAD_RENDERER_INDICES  MAX_QUAD_RENDERER_VERTICES * 6u

class QuadRendererProgram: public RendererProgram
{
public:
    struct UBO_MVP
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex
    {
        glm::vec2 position;
        glm::vec3 tint;

        static constexpr VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription {
                .binding   = 0u,
                .stride    = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };

            return bindingDescription;
        }

        static constexpr std::vector<VkVertexInputAttributeDescription> GetAttributesDescription()
        {
            std::vector<VkVertexInputAttributeDescription> attributesDescription;
            attributesDescription.resize(2);

            attributesDescription[0] = {
                .location = 0u,
                .binding  = 0u,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(Vertex, position),
            };

            attributesDescription[1] = {
                .location = 1u,
                .binding  = 0u,
                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                .offset   = offsetof(Vertex, tint),
            };

            return attributesDescription;
        }
    };

private:
    Vertex* m_VerticesMapCurrent = nullptr;
    Vertex* m_VerticesMapBegin   = nullptr;
    Vertex* m_VerticesMapEnd     = nullptr;

    bool m_VertexBufferStaged = false;

    uint32_t m_QuadCount = 0u;

    std::vector<std::unique_ptr<Buffer>> m_UBO_Camera;

public:
    QuadRendererProgram(class Device* device, VkRenderPass renderPassHandle, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent2D extent, uint32_t swapchainImageCount);

    void            Map();
    void            UnMap();
    VkCommandBuffer CreateCommandBuffer(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent, uint32_t swapchainImageIndex);

    void CreatePipeline(VkRenderPass renderPassHandle, VkExtent2D extent);

    void UpdateCamera(uint32_t framebufferIndex);

    bool TryAdvance();

    inline Vertex*  GetMapCurrent() const { return m_VerticesMapCurrent; }
    inline uint32_t GetQuadCount() const { return m_QuadCount; }
};