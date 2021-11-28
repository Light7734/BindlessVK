#pragma once

#include "Core/Base.h"
#include "Graphics/Image.h"
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
        glm::vec2 uv;

        static constexpr VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription {
                .binding   = 0u,
                .stride    = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };

            return bindingDescription;
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 3> GetAttributesDescription()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributesDescription;

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

            attributesDescription[2] = {
                .location = 2u,
                .binding  = 0u,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(Vertex, uv),
            };

            return attributesDescription;
        }
    };

private:
    //   TEMP:
    std::unique_ptr<Image> m_Image;

    Vertex* m_VerticesMapCurrent = nullptr;
    Vertex* m_VerticesMapBegin   = nullptr;
    Vertex* m_VerticesMapEnd     = nullptr;

    bool m_VertexBufferStaged = false;

    uint32_t m_QuadCount = 0u;

    std::vector<std::unique_ptr<Buffer>> m_UBO_Camera;

public:
    QuadRendererProgram(class Device* device, VkRenderPass renderPassHandle, VkExtent2D extent, uint32_t swapchainImageCount);

    void            Map();
    void            UnMap();
    VkCommandBuffer RecordCommandBuffer(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent, uint32_t swapchainImageIndex);

    void CreateCommandPool();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void CreatePipeline(VkRenderPass renderPassHandle, VkExtent2D extent);
    void CreateCommandBuffer();

    void UpdateCamera(uint32_t framebufferIndex);

    bool TryAdvance();

    inline Vertex*  GetMapCurrent() const { return m_VerticesMapCurrent; }
    inline uint32_t GetQuadCount() const { return m_QuadCount; }
};
