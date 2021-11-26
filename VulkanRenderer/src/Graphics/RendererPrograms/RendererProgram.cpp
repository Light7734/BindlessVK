#pragma once

#include "Graphics/RendererPrograms/RendererProgram.h"

#include "Graphics/Buffers.h"
#include "Graphics/Device.h"
#include "Graphics/Shader.h"

RendererProgram::RendererProgram(Device* device, uint32_t swapchainImageCount)
    : m_Device(device), m_SwapchainImageCount(swapchainImageCount)
{
}

RendererProgram::~RendererProgram()
{
    vkDestroyDescriptorSetLayout(m_Device->logical(), m_DescriptorSetLayout, nullptr);

    // destroy shader
    m_Shader.reset();

    // destroy buffers
    m_StagingVertexBuffer.reset();
    m_VertexBuffer.reset();

    m_StagingIndexBuffer.reset();
    m_IndexBuffer.reset();

    vkFreeCommandBuffers(m_Device->logical(), m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());

    // destroy pipeline
    vkDestroyPipelineLayout(m_Device->logical(), m_PipelineLayout, nullptr);
    vkDestroyPipeline(m_Device->logical(), m_Pipeline, nullptr);
}
