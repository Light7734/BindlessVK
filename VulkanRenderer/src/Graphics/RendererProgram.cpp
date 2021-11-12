#pragma once

#include "RendererProgram.h"

#include "Graphics/Shader.h"
#include "Graphics/Buffers.h"


RendererProgram::RendererProgram(DeviceContext deviceContext) :
	m_DeviceContext(deviceContext)
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

	// destroy pipeline
	vkDestroyPipelineLayout(m_DeviceContext.logical, m_PipelineLayout, nullptr);
	vkDestroyPipeline(m_DeviceContext.logical, m_Pipeline, nullptr);
}


RainbowRectRendererProgram::RainbowRectRendererProgram(DeviceContext deviceContext, VkRenderPass renderPassHandle, VkExtent2D extent) :
	RendererProgram(deviceContext)
{
	// shader
	m_Shader = std::make_unique<Shader>("res/VertexShader.glsl", "res/PixelShader.glsl", deviceContext.logical);

	// buffers
	m_VertexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(Vertex) * MAX_RENDERER_VERTICES_RAINBOWRECT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_StagingVertexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(Vertex) * MAX_RENDERER_VERTICES_RAINBOWRECT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_IndexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(uint32_t) * MAX_RENDERER_INDICES_RAINBOWRECT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_StagingIndexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(uint32_t) * MAX_RENDERER_INDICES_RAINBOWRECT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


	// pipeline layout create-info
	VkPipelineLayoutCreateInfo pipelineLayoutInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0u,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0u,
		.pPushConstantRanges = nullptr
	};
	VKC(vkCreatePipelineLayout(deviceContext.logical, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));


	// pipeline Vertex state create-info
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributesDescription = Vertex::GetAttributesDescription();

	VkPipelineVertexInputStateCreateInfo pipelineVertexStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

		.vertexBindingDescriptionCount = 1u,
		.pVertexBindingDescriptions = &bindingDescription,

		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributesDescription.size()),
		.pVertexAttributeDescriptions = attributesDescription.data()
	};

	// pipeline input-assembly state create-info
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	// viewport
	VkViewport viewport
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(extent.width),
		.height = static_cast<float>(extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	// siccors
	VkRect2D siccors
	{
		.offset = { 0, 0 },
		.extent = extent
	};

	// viewportState
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1u,
		.pViewports = &viewport,
		.scissorCount = 1u,
		.pScissors = &siccors
	};

	// pipeline rasterization state create-info
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f,
	};

	// pipeline multisample state create-info
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	// pipeline color blend attachment state
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	// pipeline color blend state create-info
	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1u,
		.pAttachments = &pipelineColorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
	};

	// dynamic state
	VkDynamicState dynamicState[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH,
	};

	// pipeline dynamic state create-info
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2u,
		.pDynamicStates = dynamicState
	};

	// graphics pipeline create-info
	VkGraphicsPipelineCreateInfo graphicsPipelineInfo
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(m_Shader->GetShaderStages().size()),
		.pStages = m_Shader->GetShaderStages().data(),
		.pVertexInputState = &pipelineVertexStateCreateInfo,
		.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
		.pViewportState = &pipelineViewportStateCreateInfo,
		.pRasterizationState = &pipelineRasterizationStateCreateInfo,
		.pMultisampleState = &pipelineMultisampleStateCreateInfo,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &pipelineColorBlendStateCreateInfo,
		.pDynamicState = nullptr,
		.layout = m_PipelineLayout,
		.renderPass = renderPassHandle,
		.subpass = 0u,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};
	VKC(vkCreateGraphicsPipelines(deviceContext.logical, VK_NULL_HANDLE, 1u, &graphicsPipelineInfo, nullptr, &m_Pipeline));
}

RainbowRectRendererProgram::~RainbowRectRendererProgram()
{
}

void* RainbowRectRendererProgram::MapVerticesBegin()
{
	return m_VertexBuffer->Map(sizeof(Vertex) * MAX_RENDERER_VERTICES_RAINBOWRECT);
}

void RainbowRectRendererProgram::MapVerticesEnd()
{
	m_VertexBuffer->Unmap();
}

void* RainbowRectRendererProgram::MapIndicesBegin()
{
	return m_IndexBuffer->Map(sizeof(uint32_t) * MAX_RENDERER_INDICES_RAINBOWRECT);
}

void RainbowRectRendererProgram::MapIndicesEnd()
{
	m_IndexBuffer->Unmap();
}
