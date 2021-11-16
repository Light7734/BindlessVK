#pragma once

#include "RendererProgram.h"

#include "Graphics/Shader.h"
#include "Graphics/Buffers.h"

RendererProgram::RendererProgram(DeviceContext deviceContext, VkCommandPool commandPool, VkQueue graphicsQueue)
	: m_DeviceContext(deviceContext), m_CommandPool(commandPool), m_GraphicsQueue(graphicsQueue)
{
}

RendererProgram::~RendererProgram()
{
	// destroy shader
	m_Shader.reset();

	// destroy buffers
	m_StagingVertexBuffer.reset();
	m_VertexBuffer.reset();

	m_StagingIndexBuffer.reset();
	m_IndexBuffer.reset();

	vkFreeCommandBuffers(m_DeviceContext.logical, m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());

	// destroy pipeline
	vkDestroyPipelineLayout(m_DeviceContext.logical, m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_DeviceContext.logical, m_Pipeline, nullptr);
}