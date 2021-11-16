#pragma once

#include "RendererPrograms.h"

template<typename VertexType>
RendererProgram<VertexType>::RendererProgram(DeviceContext deviceContext, VkCommandPool commandPool) :
	m_DeviceContext(deviceContext)
{
}

template<typename VertexType>
RendererProgram<VertexType>::~RendererProgram()
{
	// destroy shader
	m_Shader.reset();

	// destroy buffers
	m_StagingVertexBuffer.reset();
	m_VertexBuffer.reset();

	m_StagingIndexBuffer.reset();
	m_IndexBuffer.reset();

	// destroy pipeline
	vkDestroyPipelineLayout(m_DeviceContext.logical, m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_DeviceContext.logical, m_Pipeline, nullptr);
}

template<typename VertexType>
void RendererProgram<VertexType>::ResolveStagedBuffers(VkCommandPool commandPool, VkQueue graphicsQueue)
{
	if (m_VertexBufferStaged)
	{
		m_VertexBuffer->CopyBufferToSelf(m_StagingVertexBuffer.get(), sizeof(VertexType) * m_VertexCount, commandPool, graphicsQueue);
		m_VertexBufferStaged = false;
	}

	if (m_IndexBufferStaged)
	{
		m_IndexBuffer->CopyBufferToSelf(m_StagingIndexBuffer.get(), sizeof(uint32_t) * m_IndexCount, commandPool, graphicsQueue);
		m_IndexBufferStaged = false;
	}
}

template<typename VertexType>
void RendererProgram<typename VertexType>::AdvanceVertices(uint32_t count)
{
	m_VerticesMapCurrent += count;
	m_VertexCount += count;

	if (m_VerticesMapCurrent > m_VerticesMapEnd)
	{
		// #todo:
		// right now we loop back to the begin, but we should handle this more professionally... 
		m_VerticesMapCurrent = m_VerticesMapBegin;
		m_VertexCount = 0u;
		LOG(err, "{}: vertex map current passed vertex map end", __FUNCTION__);
	}
}

template<typename VertexType>
void RendererProgram<typename VertexType>::AdvanceIndices(uint32_t count)
{
	m_IndicesMapCurrent += count;
	m_IndexCount += count;

	if (m_IndicesMapCurrent > m_IndicesMapEnd)
	{
		// #todo:
		// right now we loop back to the begin, but we should handle this more professionally... 
		m_IndicesMapCurrent = m_IndicesMapBegin;
		m_IndexCount = 0u;
		LOG(err, "{}: indices map current passed indices map end", __FUNCTION__);
	}
}

template<typename VertexType>
void RendererProgram<typename VertexType>::MapVerticesBegin()
{
	m_VertexCount = 0u;
	m_VertexBufferStaged = true;

	m_VerticesMapCurrent = (VertexType*)m_StagingVertexBuffer->Map(sizeof(VertexType) * MAX_RENDERER_VERTICES_RAINBOWRECT);
	m_VerticesMapBegin = m_VerticesMapCurrent;
	m_VerticesMapEnd = m_VerticesMapCurrent + MAX_RENDERER_VERTICES_RAINBOWRECT;
}

template<typename VertexType>
void RendererProgram<typename VertexType>::MapVerticesEnd()
{
	m_VerticesMapCurrent = nullptr;
	m_VerticesMapBegin = nullptr;
	m_VerticesMapEnd = nullptr;
	m_StagingVertexBuffer->Unmap();
}

template<typename VertexType>
void RendererProgram<typename VertexType>::MapIndicesBegin()
{
	m_IndexCount = 0u;
	m_IndexBufferStaged = true;

	m_IndicesMapCurrent = (uint32_t*)m_StagingIndexBuffer->Map(sizeof(uint32_t) * MAX_RENDERER_INDICES_RAINBOWRECT);
	m_IndicesMapBegin = m_IndicesMapCurrent;
	m_IndicesMapEnd = m_IndicesMapCurrent + MAX_RENDERER_INDICES_RAINBOWRECT;
}

template<typename VertexType>
void RendererProgram<typename VertexType>::MapIndicesEnd()
{
	m_IndicesMapCurrent = nullptr;
	m_IndicesMapBegin = nullptr;
	m_IndicesMapEnd = nullptr;
	m_StagingIndexBuffer->Unmap();
}