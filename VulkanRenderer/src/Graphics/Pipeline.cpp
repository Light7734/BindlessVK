#include "Graphics/Pipeline.hpp"

#include <glm/glm.hpp>
#include <unordered_set>

Pipeline::Pipeline(PipelineCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
    , m_PhysicalDevice(createInfo.physicalDevice)
    , m_CommandPool(createInfo.commandPool)
    , m_RenderPass(createInfo.renderPass)
    , m_MaxFramesInFlight(createInfo.maxFramesInFlight)
    , m_QueueInfo(createInfo.queueInfo)
    , m_Allocator(createInfo.allocator)

{
	/////////////////////////////////////////////////////////////////////////////////
	// Create shader modules
	{
		ShaderCreateInfo shaderCreateInfo {
			m_LogicalDevice,                        // logicalDevice
			shaderc_optimization_level_performance, // optimizationLevel
			createInfo.vertexShaderPath,            // vertexPath
			createInfo.pixelShaderPath,             // pixelPath
		};

		m_Shader = std::make_unique<Shader>(shaderCreateInfo);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the pipeline layout
	{
		vk::PushConstantRange pushConstantRange {
			vk::ShaderStageFlagBits::eVertex, // stageFlags
			0u,                               // offset
			sizeof(glm::mat4) * 2u,           // size
		};

		vk::PipelineLayoutCreateInfo pipelineLayout {
			{},                                                            // flags
			static_cast<uint32_t>(createInfo.descriptorSetLayouts.size()), // setLayoutCount
			createInfo.descriptorSetLayouts.data(),                        // pSetLayouts
			0ull,                                                          // pushConstantRangeCount
			&pushConstantRange,                                            // pPushConstantRanges
		};

		m_PipelineLayout = m_LogicalDevice.createPipelineLayout(pipelineLayout, nullptr);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Specify the fixed functions' state and create the graphics pipeline
	{
		// Vertex input state
		vk::PipelineVertexInputStateCreateInfo vertexInputState {
			{},                                                          // flags
			static_cast<uint32_t>(createInfo.vertexBindingDescs.size()), // vertexBindingDescriptionCount
			createInfo.vertexBindingDescs.data(),                        // pVertexBindingDescriptions
			static_cast<uint32_t>(createInfo.vertexAttribDescs.size()),  // vertexAttributeDescriptionCount
			createInfo.vertexAttribDescs.data(),                         // pVertexAttributeDescriptions
		};

		// Input assembly state
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState {
			{},                                   // flags
			vk::PrimitiveTopology::eTriangleList, // topology
			VK_FALSE,                             // primitiveRestartEnable
		};

		// Viewport state
		vk::Viewport viewport {
			0.0f,                                                 // x
			0.0f,                                                 // y
			static_cast<float>(createInfo.viewportExtent.width),  // width
			static_cast<float>(createInfo.viewportExtent.height), // height
			0.0f,                                                 // minDepth
			1.0f,                                                 // maxDepth
		};

		vk::Rect2D scissors {
			{ 0, 0 },                 // offset
			createInfo.viewportExtent // extent
		};

		vk::PipelineViewportStateCreateInfo viewportState {
			{},        // flags
			1u,        // viewportCount
			&viewport, // pViewports
			1u,        // scissorCount
			&scissors, // pScissors
		};

		// Rasterization state
		vk::PipelineRasterizationStateCreateInfo rasterizationState {
			{},                          // flags
			VK_FALSE,                    // depthClampEnable
			VK_FALSE,                    // rasterizerDiscardEnable
			vk::PolygonMode::eFill,      // polygonMode
			vk::CullModeFlagBits::eBack, // cullMode
			vk::FrontFace::eClockwise,   // frontFace
			VK_FALSE,                    // depthBiasEnable
			0.0f,                        // depthBiasConstantFactor
			0.0f,                        // depthBiasClamp
			0.0f,                        // depthBiasSlopeFactor
			1.0f,                        // lineWidth
		};

		// Multi-sample state
		vk::PipelineMultisampleStateCreateInfo multisampleState {
			{},                     // flags
			createInfo.sampleCount, // rasterizationSamples
			VK_FALSE,               // sampleShadingEnable
			{},                     // pSampleMask
			VK_FALSE,               // alphaToCoverageEnable
			VK_FALSE,               // alphaToOneEnable
		};

		// Depth-stencil state
		vk::PipelineDepthStencilStateCreateInfo depthStencilState {
			{},                   // flags
			VK_TRUE,              // depthTestEnable
			VK_TRUE,              // depthWriteEnable
			vk::CompareOp::eLess, // depthCompareOp
			VK_FALSE,             // depthBoundsTestEnable
			VK_FALSE,             // stencilTestEnable
			{},                   // front
			{},                   // back
			0.0f,                 // minDepthBounds
			1.0,                  // maxDepthBounds
		};

		// Color blend state
		vk::PipelineColorBlendAttachmentState colorBlendAttachment {
			VK_FALSE,                           // blendEnable
			vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
			vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
			vk::BlendOp::eAdd,                  // colorBlendOp
			vk::BlendFactor::eOne,              // srcAlphaBlendFactor
			vk::BlendFactor::eZero,             // dstAlphaBlendFactor
			vk::BlendOp::eAdd,                  // alphaBlendOp

			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA, // colorWriteMask
		};

		vk::PipelineColorBlendStateCreateInfo colorBlendState {
			{},                         // flags
			VK_FALSE,                   // logicOpEnable
			vk::LogicOp::eCopy,         // logicOp
			1u,                         // attachmentCount
			&colorBlendAttachment,      // pAttachments
			{ 0.0f, 0.0f, 0.0f, 0.0f }, // blendConstants
		};

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
			{},                                    // flags
			m_Shader->GetStageCount(),             // stageCount
			m_Shader->GetShaderStageCreateInfos(), // pStages
			&vertexInputState,                     // pVertexInputState
			&inputAssemblyState,                   // pInputAssemblyState
			nullptr,                               // pTessalationState
			&viewportState,                        // pViewportState
			&rasterizationState,                   // pRasterizationState
			&multisampleState,                     // pMultisampleState
			&depthStencilState,                    // pDepthStencilState
			&colorBlendState,                      // pColorBlendState
			nullptr,                               // pDynamicState
			m_PipelineLayout,                      // layout
			m_RenderPass,                          // renderPass
			0u,                                    // subpass
			{},                                    // basePipelineHandle
			-1,                                    // basePipelineIndex
		};

		vk::ResultValue<vk::Pipeline> pipeline = m_LogicalDevice.createGraphicsPipeline({}, graphicsPipelineCreateInfo);
		VKC(pipeline.result);

		m_Pipeline = pipeline.value;
	}
}

Pipeline::~Pipeline()
{
	m_Shader.reset();
	m_LogicalDevice.destroyDescriptorSetLayout(m_DescriptorSetLayout, nullptr);
	m_LogicalDevice.destroyPipelineLayout(m_PipelineLayout, nullptr);
	m_LogicalDevice.destroyPipeline(m_Pipeline, nullptr);
}

UUID Pipeline::CreateRenderable(RenderableCreateInfo& createInfo)
{
	m_Renderables.emplace_back(createInfo);
	LOG(trace, "Added renderable with id: {}", m_Renderables.back().m_Id);
	return m_Renderables.back().m_Id;
}

void Pipeline::RemoveRenderable(UUID renderableId)
{
	auto it = std::find_if(m_Renderables.begin(), m_Renderables.end(), [renderableId](const Renderable& renderable) { return renderable.m_Id == renderableId; });

	if (it != m_Renderables.end())
	{
		m_Renderables.erase(it);
	}

	LOG(trace, "Removed renderable with id: {}", renderableId);
}

void Pipeline::RecreateBuffers()
{
	m_IndicesCount = 0u;
	m_VertexBuffer.reset();
	m_IndexBuffer.reset();
	m_StorageBuffer.reset();

	std::vector<Vertex> vertices;
	std::vector<ObjectData> objects;
	std::vector<uint32_t> indices;
	std::unordered_set<std::shared_ptr<Texture>> textures;

	uint32_t index = 0;
	for (auto& renderable : m_Renderables)
	{
		for (auto& vertex : renderable.GetVertices())
			vertex.objectIndex = index;

		vertices.insert(vertices.end(), renderable.GetVertices().begin(), renderable.GetVertices().end());
		indices.insert(indices.end(), renderable.GetIndices().begin(), renderable.GetIndices().end());

		m_IndicesCount += renderable.GetIndicesCount();

		// #TODO
		objects.push_back(ObjectData { .transform = glm::mat4(1.0) });
	}

	// vertex buffer
	BufferCreateInfo vbufferCreateInfo {
		.logicalDevice  = m_LogicalDevice,
		.physicalDevice = m_PhysicalDevice,
		.allocator      = m_Allocator,
		.commandPool    = m_CommandPool,
		.graphicsQueue  = m_QueueInfo.graphicsQueue,
		.usage          = vk::BufferUsageFlagBits::eVertexBuffer,
		.size           = vertices.size() * sizeof(Vertex),
		.initialData    = vertices.data(),
	};
	m_VertexBuffer = std::make_unique<StagingBuffer>(vbufferCreateInfo);

	// index buffer
	BufferCreateInfo ibufferCreateInfo {
		.logicalDevice  = m_LogicalDevice,
		.physicalDevice = m_PhysicalDevice,
		.allocator      = m_Allocator,
		.commandPool    = m_CommandPool,
		.graphicsQueue  = m_QueueInfo.graphicsQueue,
		.usage          = vk::BufferUsageFlagBits::eIndexBuffer,
		.size           = indices.size() * sizeof(uint32_t),
		.initialData    = indices.data(),
	};
	m_IndexBuffer = std::make_unique<StagingBuffer>(ibufferCreateInfo);
}

void Pipeline::RecordCommandBuffer(vk::CommandBuffer cmd)
{
	// Bind pipeline
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);

	// Bind vertex & index buffer
	static VkDeviceSize offset { 0 };

	cmd.bindVertexBuffers(0, 1, m_VertexBuffer->GetBuffer(), &offset);
	cmd.bindIndexBuffer(*m_IndexBuffer->GetBuffer(), 0u, vk::IndexType::eUint32);

	// Bind pipeline descriptors
	// ...

	// Push constants
	// ...

	// Draw
	cmd.drawIndexed(m_IndicesCount, 1u, 0u, 0u, 0u);
}
