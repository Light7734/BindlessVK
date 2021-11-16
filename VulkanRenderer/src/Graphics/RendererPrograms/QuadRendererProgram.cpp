#include "QuadRendererProgram.h"

QuadRendererProgram::QuadRendererProgram(DeviceContext deviceContext, VkRenderPass renderPassHandle, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent2D extent, uint32_t swapchainImageCount) :
	RendererProgram(deviceContext, commandPool, graphicsQueue)
{
	// shader
	m_Shader = std::make_unique<Shader>("res/VertexShader.glsl", "res/PixelShader.glsl", deviceContext.logical);

	// buffers
	m_VertexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(Vertex) * MAX_QUAD_RENDERER_VERTICES, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_StagingVertexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(Vertex) * MAX_QUAD_RENDERER_VERTICES, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_IndexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(uint32_t) * MAX_QUAD_RENDERER_INDICES, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_StagingIndexBuffer = std::make_unique<class Buffer>(deviceContext, sizeof(uint32_t) * MAX_QUAD_RENDERER_INDICES, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// create indices
	uint32_t* indices = new uint32_t[MAX_QUAD_RENDERER_INDICES];
	uint32_t offest = 0u;
	for (uint32_t i = 0u; i < MAX_QUAD_RENDERER_INDICES; i += 6)
	{
		indices[i + 0] = offest + 0;
		indices[i + 1] = offest + 1;
		indices[i + 2] = offest + 2;

		indices[i + 3] = offest + 2;
		indices[i + 4] = offest + 3;
		indices[i + 5] = offest + 0;

		offest += 4u;
	}

	// copy indices over
	uint32_t* map = (uint32_t*)m_StagingIndexBuffer->Map(sizeof(uint32_t) * MAX_QUAD_RENDERER_INDICES);
	memcpy(map, indices, sizeof(uint32_t) * MAX_QUAD_RENDERER_INDICES);
	m_StagingIndexBuffer->Unmap();
	m_IndexBuffer->CopyBufferToSelf(m_StagingIndexBuffer.get(), sizeof(uint32_t) * MAX_QUAD_RENDERER_INDICES, m_CommandPool, m_GraphicsQueue);

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


	// create render pipeline
	CreatePipeline(renderPassHandle, extent);

	// command buffer allocate-info
	VkCommandBufferAllocateInfo commandBufferAllocateInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = swapchainImageCount,
	};
	m_CommandBuffers.resize(swapchainImageCount);

	VKC(vkAllocateCommandBuffers(deviceContext.logical, &commandBufferAllocateInfo, m_CommandBuffers.data()));
}

void QuadRendererProgram::Map()
{
	m_QuadCount = 0u;
	m_VerticesMapCurrent = (Vertex*)m_StagingVertexBuffer->Map(sizeof(Vertex) * MAX_QUAD_RENDERER_VERTICES);

	m_VerticesMapBegin = m_VerticesMapCurrent;
	m_VerticesMapEnd = m_VerticesMapCurrent + MAX_QUAD_RENDERER_VERTICES;
}

void QuadRendererProgram::UnMap()
{
	m_StagingVertexBuffer->Unmap();

	m_VerticesMapCurrent = nullptr;
	m_VerticesMapBegin = nullptr;
	m_VerticesMapEnd = nullptr;
}

VkCommandBuffer QuadRendererProgram::CreateCommandBuffer(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent, uint32_t swapchainImageIndex)
{
	const VkDeviceSize offsets[] = { 0 };

	// command buffer begin-info
	static const VkCommandBufferBeginInfo  commandBufferBeginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = NULL,
		.pInheritanceInfo = nullptr,
	};

	// clear value
	static const VkClearValue clearColor =
	{
		.color = { 0.124, 0.231, 0.491f, 1.0f }
	};

	// render pass begin-info
	VkRenderPassBeginInfo  renderpassBeginInfo
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = frameBuffer,

		.renderArea =
		{
			.offset = {0, 0},
			.extent = swapchainExtent
		},

		.clearValueCount = 1u,
		.pClearValues = &clearColor
	};

	// alias
	const auto& cmd = m_CommandBuffers[swapchainImageIndex];

	VKC(vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
	VKC(vkBeginCommandBuffer(cmd, &commandBufferBeginInfo));
	const VkBufferCopy copyRegion
	{
		.srcOffset = 0u,
		.dstOffset = 0u,
		.size = m_QuadCount * 4u * sizeof(Vertex)
	};
	vkCmdCopyBuffer(cmd, *m_StagingVertexBuffer->GetBuffer(), *m_VertexBuffer->GetBuffer(), 1u, &copyRegion);

	vkCmdBeginRenderPass(cmd, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	vkCmdBindVertexBuffers(cmd, 0u, 1u, m_VertexBuffer->GetBuffer(), offsets);
	vkCmdBindIndexBuffer(cmd, *m_IndexBuffer->GetBuffer(), 0u, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, m_QuadCount * 6u, m_QuadCount, 0u, 0u, 0u);

	vkCmdEndRenderPass(cmd);

	VKC(vkEndCommandBuffer(cmd));

	return cmd;
}

void QuadRendererProgram::CreatePipeline(VkRenderPass renderPassHandle, VkExtent2D extent)
{
	// pipeline Vertex state create-info
	auto bindingDescription = QuadRendererProgram::Vertex::GetBindingDescription();
	auto attributesDescription = QuadRendererProgram::Vertex::GetAttributesDescription();

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

	VKC(vkCreateGraphicsPipelines(m_DeviceContext.logical, VK_NULL_HANDLE, 1u, &graphicsPipelineInfo, nullptr, &m_Pipeline));
}

bool QuadRendererProgram::TryAdvance()
{
	m_VerticesMapCurrent += 4;
	m_QuadCount++;

	return m_VerticesMapCurrent < m_VerticesMapEnd;
}
