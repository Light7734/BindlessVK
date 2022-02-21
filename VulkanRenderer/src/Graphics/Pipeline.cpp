#include "Graphics/Pipeline.hpp"

Pipeline::Pipeline(PipelineCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
{
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
	// Create the pipeline layout
	{
		VkPipelineLayoutCreateInfo pipelineLayout {
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount         = 0u,
			.pSetLayouts            = nullptr,
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
			.vertexBindingDescriptionCount   = 0u,
			.pVertexBindingDescriptions      = nullptr,
			.vertexAttributeDescriptionCount = 0u,
			.pVertexAttributeDescriptions    = nullptr,
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
			.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable   = VK_FALSE,
			.pSampleMask           = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable      = VK_FALSE,
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

		// Dynamic states
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
		};

		VkPipelineDynamicStateCreateInfo dynamicState {
			.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = 2u,
			.pDynamicStates    = dynamicStates,
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
			.pDepthStencilState  = nullptr,
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
}

VkCommandBuffer Pipeline::RecordCommandBuffer(CommandBufferStartInfo& startInfo)
{
	uint32_t index = startInfo.imageIndex; // alias

	// Begin command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo {
		.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags            = 0x0,
		.pInheritanceInfo = nullptr
	};

	VKC(vkBeginCommandBuffer(m_CommandBuffers[index], &commandBufferBeginInfo));

	// Begin renderpass
	VkClearValue clearValue { { {
		0.0f,
		0.0f,
		0.0f,
		1.0f,
	} } };

	VkRenderPassBeginInfo renderpassBeginInfo {
		.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = startInfo.renderPass,
		.framebuffer = startInfo.framebuffer,
		.renderArea {
		    .offset = { 0, 0 },
		    .extent = startInfo.extent,
		},
		.clearValueCount = 1u,
		.pClearValues    = &clearValue,
	};

	vkCmdBeginRenderPass(m_CommandBuffers[index], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind pipeline
	vkCmdBindPipeline(m_CommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

	// Draw
	vkCmdDraw(m_CommandBuffers[index], 3, 1, 0, 0);

	// End...
	vkCmdEndRenderPass(m_CommandBuffers[index]);
	VKC(vkEndCommandBuffer(m_CommandBuffers[index]));

	return m_CommandBuffers[index];
}

Pipeline::~Pipeline()
{
	vkDestroyPipeline(m_LogicalDevice, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
}
