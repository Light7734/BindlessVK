#pragma once

#include "Core/Base.h"
#include "Graphics/Image.h"
#include "Graphics/RendererPrograms/RendererProgram.h"

#include <volk.h>

#define MAX_MODEL_RENDERER_VERTICES 100000u * 4u
#define MAX_MODEL_RENDERER_INDICES  MAX_MODEL_RENDERER_VERTICES * 6u

class ModelRendererProgram: public RendererProgram
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
		glm::vec3 position;
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

		static constexpr std::array<VkVertexInputAttributeDescription, 2> GetAttributesDescription()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributesDescription;

			attributesDescription[0] = {
				.location = 0u,
				.binding  = 0u,
				.format   = VK_FORMAT_R32G32B32_SFLOAT,
				.offset   = offsetof(Vertex, position),
			};

			attributesDescription[1] = {
				.location = 1u,
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

	uint32_t m_ModelCount  = 0u;
	uint32_t m_VertexCount = 0u;

	std::vector<std::unique_ptr<Buffer>> m_UBO_Camera;


	// Depth resources
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	// Model image
	VkImageView m_ModelImageView;
	VkSampler m_ModelSampler;

public:
	ModelRendererProgram(class Device* device, VkRenderPass renderPassHandle, VkExtent2D extent, uint32_t swapchainImageCount);

	void Map();
	void UnMap();
	VkCommandBuffer RecordCommandBuffer(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent, uint32_t swapchainImageIndex);

	void UpdateCamera(uint32_t framebufferIndex);
	void UpdateImage(uint32_t framebufferIndex);

	inline void SetImage(VkImageView imageView, VkSampler sampler)
	{
		m_ModelImageView = imageView;
		m_ModelSampler   = sampler;
	}

	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void CreatePipeline(VkRenderPass renderPassHandle, VkExtent2D extent);
	void CreateCommandBuffer();

	bool TryAdvance(size_t vertexCount);

	inline Vertex* GetMapCurrent() const { return m_VerticesMapCurrent; }
	inline uint32_t GetModelCount() const { return m_ModelCount; }

private:
	void CreatePipelineLayout();
};
