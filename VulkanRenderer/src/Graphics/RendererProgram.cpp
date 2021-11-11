#pragma once

#include "RendererProgram.h"

RendererProgram::~RendererProgram()
{
	// destroy shader
	m_Shader.reset();

	// destroy buffers
	m_StagingVertexBuffer.reset();
	m_VertexBuffer.reset();

	m_StagingIndexBuffer.reset();
	m_IndexBuffer.reset();

	// destroy pipeline
	vkDestroyPipeline(m_DeviceContext.logical, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_DeviceContext.logical, m_PipelineLayout, nullptr);
}