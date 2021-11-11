#pragma once

#include "Core/Base.h"

#include "Graphics/Shader.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Buffers.h"


#include <volk.h>

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

protected:
	RendererProgram() = default;

	virtual ~RendererProgram();
};
