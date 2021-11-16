#pragma once	

#include "Core/Base.h"

#include "Graphics/Buffers.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/RendererProgramsVertexTypes.h"
#include "Graphics/Shader.h"

#include <volk.h>

#define MAX_RENDERER_VERTICES_RAINBOWRECT 1000ull
#define MAX_RENDERER_INDICES_RAINBOWRECT MAX_RENDERER_VERTICES_RAINBOWRECT * 6ull

template<typename VertexType>
class RendererProgram
{
protected:
	DeviceContext m_DeviceContext;

	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

	VkViewport m_Viewport;
	VkRect2D m_Scissors;

	std::unique_ptr<Shader> m_Shader = nullptr;

	std::unique_ptr<Buffer> m_VertexBuffer = nullptr;
	std::unique_ptr<Buffer> m_IndexBuffer = nullptr;

	std::unique_ptr<Buffer> m_StagingVertexBuffer = nullptr;
	std::unique_ptr<Buffer> m_StagingIndexBuffer = nullptr;

	VertexType* m_VerticesMapCurrent = nullptr;
	VertexType* m_VerticesMapBegin = nullptr;
	VertexType* m_VerticesMapEnd = nullptr;

	uint32_t* m_IndicesMapBegin = nullptr;
	uint32_t* m_IndicesMapCurrent = nullptr;
	uint32_t* m_IndicesMapEnd = nullptr;

	bool m_VertexBufferStaged = false;
	bool m_IndexBufferStaged = false;

	uint32_t m_VertexCount = 0u;
	uint32_t m_IndexCount = 0u;

public:
	RendererProgram(DeviceContext deviceContext, VkCommandPool commandPool);

	virtual ~RendererProgram();

	void ResolveStagedBuffers(VkCommandPool commandPool, VkQueue graphicsQueue);

	void AdvanceVertices(uint32_t count);
	void AdvanceIndices(uint32_t count);

	void MapVerticesBegin();
	void MapVerticesEnd();

	void MapIndicesBegin();
	void MapIndicesEnd();

	FORCEINLINE VkPipeline GetPipeline() { return m_Pipeline; }

	FORCEINLINE const VkBuffer* GetVertexVkBuffer() const { return m_VertexBuffer->GetBuffer(); }
	FORCEINLINE const VkBuffer* GetIndexVkBuffer() const { return m_IndexBuffer->GetBuffer(); }

	FORCEINLINE VertexType* GetVerticesMapCurrent() { return m_VerticesMapCurrent; }
	FORCEINLINE uint32_t* GetIndicesMapCurrent() { return m_IndicesMapCurrent; }

	FORCEINLINE uint32_t GetVertexCount() const { return m_VertexCount; }
	FORCEINLINE uint32_t GetIndexCount() const { return m_IndexCount; }
};

class RainbowRectRendererProgram : public RendererProgram<RainbowRectVertex>
{
public:
	RainbowRectRendererProgram(DeviceContext deviceContext, VkRenderPass renderPassHandle, VkCommandPool commandPool, VkExtent2D extent);
};

#include "RendererProgramsImp.h"