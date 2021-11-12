#pragma once	

#include "Core/Base.h"

#include "Graphics/DeviceContext.h"

#include "Graphics/Buffers.h"
#include "Graphics/Shader.h"

#include <volk.h>

#define MAX_RENDERER_VERTICES_RAINBOWRECT 1000ull
#define MAX_RENDERER_INDICES_RAINBOWRECT MAX_RENDERER_VERTICES_RAINBOWRECT * 6ull


class RendererProgram
{
protected:
	DeviceContext m_DeviceContext;

	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout;

	VkViewport m_Viewport;
	VkRect2D m_Scissors;

	std::unique_ptr<Shader> m_Shader;

	std::unique_ptr<Buffer> m_VertexBuffer;
	std::unique_ptr<Buffer> m_IndexBuffer;

	std::unique_ptr<Buffer> m_StagingVertexBuffer;
	std::unique_ptr<Buffer> m_StagingIndexBuffer;

	uint32_t m_Count = 0;

public:
	RendererProgram(DeviceContext deviceContext);
	virtual ~RendererProgram();

	FORCEINLINE VkPipeline GetPipeline() { return m_Pipeline; }

	virtual void* MapVerticesBegin() = 0;
	virtual void MapVerticesEnd() = 0;

	virtual void* MapIndicesBegin() = 0;
	virtual void MapIndicesEnd() = 0;

	FORCEINLINE const VkBuffer* GetVertexVkBuffer() const { return m_VertexBuffer->GetBuffer(); }
	FORCEINLINE const VkBuffer* GetIndexVkBuffer() const { return m_IndexBuffer->GetBuffer(); }

	FORCEINLINE uint32_t GetCount() const { return m_Count; }
};

class RainbowRectRendererProgram : public RendererProgram
{
public:
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;

		constexpr static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription
			{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			};

			return bindingDescription;
		}

		constexpr static std::array<VkVertexInputAttributeDescription, 2> GetAttributesDescription()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributesDescription;

			attributesDescription[0] =
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, position),
			};

			attributesDescription[1] =
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, color),
			};

			return attributesDescription;
		}
	};

public:
	RainbowRectRendererProgram(DeviceContext deviceContext, VkRenderPass renderPassHandle, VkExtent2D extent);
	~RainbowRectRendererProgram();

	void* MapVerticesBegin() override;
	void MapVerticesEnd() override;

	void* MapIndicesBegin() override;
	void MapIndicesEnd() override;
};