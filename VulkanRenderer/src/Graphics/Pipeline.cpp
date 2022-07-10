#include "Graphics/Pipeline.hpp"

#include <glm/glm.hpp>

Pipeline::Pipeline(PipelineCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_RenderPass(createInfo.renderPass), m_Model(createInfo.model)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create command buffers...
	{
		m_CommandBuffers.resize(createInfo.imageCount);

		VkCommandBufferAllocateInfo commandBufferAllocInfo {
			.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool        = createInfo.commandPool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size()),
		};

		VKC(vkAllocateCommandBuffers(m_LogicalDevice, &commandBufferAllocInfo, m_CommandBuffers.data()));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create shader modules
	{
		ShaderCreateInfo shaderCreateInfo {
			.logicalDevice     = m_LogicalDevice,
			.optimizationLevel = shaderc_optimization_level_performance,
			.vertexPath        = createInfo.vertexShaderPath,
			.pixelPath         = createInfo.pixelShaderPath,
		};

		m_Shader = std::make_unique<Shader>(shaderCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create vertex & index buffers
	{
		BufferCreateInfo vertexBufferCreateInfo {
			.logicalDevice  = m_LogicalDevice,
			.physicalDevice = createInfo.physicalDevice,
			.commandPool    = createInfo.commandPool,
			.graphicsQueue  = createInfo.graphicsQueue,
			.usage          = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.size           = createInfo.model->GetVerticesSize(),
			.initialData    = createInfo.model->GetVertices(),
		};

		BufferCreateInfo indexBufferCreateInfo {
			.logicalDevice  = m_LogicalDevice,
			.physicalDevice = createInfo.physicalDevice,
			.commandPool    = createInfo.commandPool,
			.graphicsQueue  = createInfo.graphicsQueue,
			.usage          = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			.size           = createInfo.model->GetIndicesSize(),
			.initialData    = createInfo.model->GetIndices(),
		};

		m_VertexBuffer = std::make_unique<StagingBuffer>(vertexBufferCreateInfo);
		m_IndexBuffer  = std::make_unique<StagingBuffer>(indexBufferCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the pipeline layout
	{
		VkPipelineLayoutCreateInfo pipelineLayout {
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount         = static_cast<uint32_t>(createInfo.descriptorSetLayouts.size()),
			.pSetLayouts            = createInfo.descriptorSetLayouts.data(),
			.pushConstantRangeCount = 0u,
			.pPushConstantRanges    = nullptr,
		};

		VKC(vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayout, nullptr, &m_PipelineLayout));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Specify the fixed functions' state and create the graphics pipeline
	{
		// Vertex input state
		VkPipelineVertexInputStateCreateInfo vertexInputState {
			.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount   = 1u,
			.pVertexBindingDescriptions      = &createInfo.vertexBindingDesc,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(createInfo.vertexAttribDescs.size()),
			.pVertexAttributeDescriptions    = createInfo.vertexAttribDescs.data(),
		};

		// Input assembly state
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		// Viewport state
		VkViewport viewport {
			.x        = 0.0f,
			.y        = 0.0f,
			.width    = static_cast<float>(createInfo.viewportExtent.width),
			.height   = static_cast<float>(createInfo.viewportExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissors {
			.offset = { 0, 0 },
			.extent = createInfo.viewportExtent
		};

		VkPipelineViewportStateCreateInfo viewportState {
			.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1u,
			.pViewports    = &viewport,
			.scissorCount  = 1u,
			.pScissors     = &scissors,
		};

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationState {
			.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable        = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode             = VK_POLYGON_MODE_FILL,
			.cullMode                = VK_CULL_MODE_BACK_BIT,
			.frontFace               = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable         = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp          = 0.0f,
			.depthBiasSlopeFactor    = 0.0f,
			.lineWidth               = 1.0f,
		};

		// Multi-sample state
		VkPipelineMultisampleStateCreateInfo multisampleState {
			.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples  = createInfo.sampleCount,
			.sampleShadingEnable   = VK_FALSE,
			.pSampleMask           = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable      = VK_FALSE,
		};

		// Depth-stencil state
		VkPipelineDepthStencilStateCreateInfo depthStencilState {
			.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable       = VK_TRUE,
			.depthWriteEnable      = VK_TRUE,
			.depthCompareOp        = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable     = VK_FALSE,
			.front                 = {},
			.back                  = {},
			.minDepthBounds        = 0.0f,
			.maxDepthBounds        = 1.0,
		};

		// Color blend state
		VkPipelineColorBlendAttachmentState colorBlendAttachment {
			.blendEnable         = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp        = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp        = VK_BLEND_OP_ADD,
			.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlendState {
			.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable   = VK_FALSE,
			.logicOp         = VK_LOGIC_OP_COPY,
			.attachmentCount = 1u,
			.pAttachments    = &colorBlendAttachment,
			.blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f },
		};

		// Create the graphics pipeline
		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
			.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount          = m_Shader->GetStageCount(),
			.pStages             = m_Shader->GetShaderStageCreateInfos(),
			.pVertexInputState   = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState      = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState   = &multisampleState,
			.pDepthStencilState  = &depthStencilState,
			.pColorBlendState    = &colorBlendState,
			.pDynamicState       = nullptr,
			.layout              = m_PipelineLayout,
			.renderPass          = m_RenderPass,
			.subpass             = 0u,
			.basePipelineHandle  = VK_NULL_HANDLE,
			.basePipelineIndex   = -1,
		};

		VKC(vkCreateGraphicsPipelines(m_LogicalDevice, nullptr, 1u, &graphicsPipelineCreateInfo, nullptr, &m_Pipeline));
	}
}

Pipeline::~Pipeline()
{
	vkDestroyPipeline(m_LogicalDevice, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
}

VkCommandBuffer Pipeline::RecordCommandBuffer(CommandBufferStartInfo& startInfo)
{
	uint32_t index = startInfo.frameIndex; // alias
	VKC(vkResetCommandBuffer(m_CommandBuffers[index], 0));

	// Begin command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo {
		.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags            = 0x0,
		.pInheritanceInfo = nullptr
	};

	VKC(vkBeginCommandBuffer(m_CommandBuffers[index], &commandBufferBeginInfo));

	// Begin renderpass
	std::vector<VkClearValue> clearValues;
	clearValues.push_back(VkClearValue { .color = { 0.0f, 0.0f, 0.0f, 1.0f } });
	clearValues.push_back(VkClearValue { .depthStencil = { 1.0f, 0 } });

	VkRenderPassBeginInfo renderpassBeginInfo {
		.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = m_RenderPass,
		.framebuffer = startInfo.framebuffer,
		.renderArea {
		    .offset = { 0, 0 },
		    .extent = startInfo.extent,
		},
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues    = clearValues.data(),
	};

	vkCmdBeginRenderPass(m_CommandBuffers[index], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind pipeline
	vkCmdBindPipeline(m_CommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	// Bind vertex & index buffer
	static VkDeviceSize offset { 0 };
	vkCmdBindVertexBuffers(m_CommandBuffers[index], 0, 1, m_VertexBuffer->GetBuffer(), &offset);
	vkCmdBindIndexBuffer(m_CommandBuffers[index], *m_IndexBuffer->GetBuffer(), 0u, VK_INDEX_TYPE_UINT32);

	// Bind descriptor sets
	vkCmdBindDescriptorSets(m_CommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0u, 1u, startInfo.mvpDescriptorSet, 0, nullptr);

	// Draw
	// vkCmdDraw(m_CommandBuffers[index], 3, 1, 0, 0);
	vkCmdDrawIndexed(m_CommandBuffers[index], m_Model->GetIndicesCount(), 1u, 0u, 0u, 0u);

	// End...
	vkCmdEndRenderPass(m_CommandBuffers[index]);
	VKC(vkEndCommandBuffer(m_CommandBuffers[index]));

	return m_CommandBuffers[index];
}
