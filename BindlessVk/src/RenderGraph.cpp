#include "BindlessVk/RenderGraph.hpp"

#include "BindlessVk/Texture.hpp"

namespace BINDLESSVK_NAMESPACE {

RenderGraph::RenderGraph()
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::Init(const CreateInfo& info)
{
	m_Device              = info.device;
	m_DescriptorPool      = info.descriptorPool;
	m_CommandPool         = info.commandPool;
	m_SwapchainImages     = info.swapchainImages;
	m_SwapchainImageViews = info.swapchainImageViews;
}

void RenderGraph::Build(const BuildInfo& info)
{
	m_RenderPassCreateInfos = info.renderPasses;
	m_BufferInputInfos      = info.bufferInputs;
	m_OnUpdate              = info.onUpdate;
	m_OnBeginFrame          = info.onBeginFrame;

	m_SwapchainAttachmentNames.push_back(info.backbufferName);
	m_RenderPasses.resize(m_RenderPassCreateInfos.size(), RenderPass {});

	CreateCommandBuffers();

	ValidateGraph();
	ReorderPasses();

	BuildAttachmentResources();

	BuildTextureInputs();
	BuildBufferInputs();

	BuildDescriptorSets();
	WriteDescriptorSets();
}

void RenderGraph::CreateCommandBuffers()
{
	vk::CommandBufferAllocateInfo cmdBufferallocInfo {
		m_CommandPool,
		vk::CommandBufferLevel::eSecondary,
		MAX_FRAMES_IN_FLIGHT * (uint32_t)m_RenderPassCreateInfos.size(),
	};

	m_SecondaryCommandBuffers = m_Device->logical.allocateCommandBuffers(cmdBufferallocInfo);
}

// @todo: Implement
void RenderGraph::ValidateGraph()
{
}

// @todo: Implement
void RenderGraph::ReorderPasses()
{
	for (uint32_t i = m_RenderPassCreateInfos.size(); i-- > 0;)
	{
		m_RenderPasses[i].name         = m_RenderPassCreateInfos[i].name;
		m_RenderPasses[i].onUpdate     = m_RenderPassCreateInfos[i].onUpdate;
		m_RenderPasses[i].onRender     = m_RenderPassCreateInfos[i].onRender;
		m_RenderPasses[i].onBeginFrame = m_RenderPassCreateInfos[i].onBeginFrame;


		for (const RenderPass::CreateInfo::AttachmentInfo& info : m_RenderPassCreateInfos[i].colorAttachmentInfos)
		{
			auto it = std::find(m_SwapchainAttachmentNames.begin(),
			                    m_SwapchainAttachmentNames.end(),
			                    info.name);

			if (it != m_SwapchainAttachmentNames.end())
			{
				m_SwapchainAttachmentNames.push_back(info.input);
			}
		}
	}
}

void RenderGraph::BuildAttachmentResources()
{
	uint32_t passIndex = 0u;
	for (const RenderPass::CreateInfo& renderPassCreateInfo : m_RenderPassCreateInfos)
	{
		RenderPass& pass = m_RenderPasses[passIndex++];

		for (const RenderPass::CreateInfo::AttachmentInfo& info : renderPassCreateInfo.colorAttachmentInfos)
		{
			// Write only ( create new image )
			if (info.input == "")
			{
				auto it = std::find(m_SwapchainAttachmentNames.begin(),
				                    m_SwapchainAttachmentNames.end(),
				                    info.input);


				CreateAttachmentResource(info, it != m_SwapchainAttachmentNames.end() ?
				                                   AttachmentResourceContainer::Type::ePerImage :
				                                   AttachmentResourceContainer::Type::eSingle);

				pass.attachments.push_back({
				    .stageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
				    .accessMask = vk::AccessFlagBits::eColorAttachmentWrite,
				    .layout     = vk::ImageLayout::eColorAttachmentOptimal,
				    /* subresourceRange */
				    .subresourceRange = vk::ImageSubresourceRange {
				        vk::ImageAspectFlagBits::eColor, // aspectMask
				        0u,                              // baseMipLevel
				        1u,                              // levelCount
				        0u,                              // baseArrayLayer
				        1u,                              // layerCount
				    },

				    .loadOp        = vk::AttachmentLoadOp::eClear,
				    .storeOp       = vk::AttachmentStoreOp::eStore,
				    .resourceIndex = static_cast<uint32_t>(m_AttachmentResources.size() - 1u),
				    .clearValue    = info.clearValue,
				});
			}
			// Read-Modify-Write ( alias the image )
			else
			{
				uint32_t i = 0;
				for (AttachmentResourceContainer& attachmentResource : m_AttachmentResources)
				{
					if (attachmentResource.size != info.size || attachmentResource.sizeType != info.sizeType)
					{
						BVK_ASSERT(ErrorCodes::eUnsupported, "ReadWrite attachment with different size from input is currently not supported");
					}

					if (attachmentResource.lastWriteName == info.input)
					{
						attachmentResource.lastWriteName = info.name;

						pass.attachments.push_back({
						    .stageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
						    .accessMask = vk::AccessFlagBits::eColorAttachmentWrite,
						    .layout     = vk::ImageLayout::eColorAttachmentOptimal,
						    /* subresourceRange */
						    .subresourceRange = vk::ImageSubresourceRange {
						        vk::ImageAspectFlagBits::eColor, // aspectMask
						        0u,                              // baseMipLevel
						        1u,                              // levelCount
						        0u,                              // baseArrayLayer
						        1u,                              // layerCount
						    },

						    .loadOp        = vk::AttachmentLoadOp::eLoad,
						    .storeOp       = vk::AttachmentStoreOp::eStore,
						    .resourceIndex = i,
						});
						break;
					}

					i++;
				}
			}
		}

		if (!renderPassCreateInfo.depthStencilAttachmentInfo.name.empty())
		{
			const RenderPass::CreateInfo::AttachmentInfo& info = renderPassCreateInfo.depthStencilAttachmentInfo;
			// Write only ( create new image )
			if (info.input == "")
			{
				auto it = std::find(m_SwapchainAttachmentNames.begin(),
				                    m_SwapchainAttachmentNames.end(),
				                    info.input);


				CreateAttachmentResource(info, AttachmentResourceContainer::Type::eSingle);

				pass.attachments.push_back({
				    .stageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests,
				    .accessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead,
				    .layout     = vk::ImageLayout::eDepthAttachmentOptimal,
				    /* subresourceRange */
				    .subresourceRange = vk::ImageSubresourceRange {
				        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, // aspectMask

				        0u, // baseMipLevel
				        1u, // levelCount
				        0u, // baseArrayLayer
				        1u, // layerCount
				    },

				    .loadOp  = vk::AttachmentLoadOp::eClear,
				    .storeOp = vk::AttachmentStoreOp::eStore,

				    .resourceIndex = static_cast<uint32_t>(m_AttachmentResources.size() - 1u),
				    .clearValue    = info.clearValue,
				});
			}
			// Read-Modify-Write ( alias the image )
			else
			{
				uint32_t i = 0;
				for (AttachmentResourceContainer& attachmentResource : m_AttachmentResources)
				{
					if (attachmentResource.size != info.size || attachmentResource.sizeType != info.sizeType)
					{
						BVK_ASSERT(true, "ReadWrite attachment with different size from input is currently not supported");
					}

					if (attachmentResource.lastWriteName == info.input)
					{
						attachmentResource.lastWriteName = info.name;

						pass.attachments.push_back({
						    .stageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests,
						    .accessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead,
						    .layout     = vk::ImageLayout::eDepthAttachmentOptimal,
						    /* subresourceRange */
						    .subresourceRange = vk::ImageSubresourceRange {
						        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, // aspectMask

						        0u, // baseMipLevel
						        1u, // levelCount
						        0u, // baseArrayLayer
						        1u, // layerCount
						    },

						    .loadOp        = vk::AttachmentLoadOp::eLoad,
						    .storeOp       = vk::AttachmentStoreOp::eStore,
						    .resourceIndex = 1u,
						});
						break;
					}

					i++;
				}
			}
		}
	}
}


void RenderGraph::BuildTextureInputs()
{
}

void RenderGraph::BuildBufferInputs()
{
	for (RenderPass::CreateInfo::BufferInputInfo& info : m_BufferInputInfos)
	{
		BufferCreateInfo bufferCreateInfo {
			.device       = m_Device,
			.commandPool  = m_CommandPool,
			.usage        = info.type == vk::DescriptorType::eUniformBuffer ? vk::BufferUsageFlagBits::eUniformBuffer :
			                                                                  vk::BufferUsageFlagBits::eStorageBuffer,
			.minBlockSize = info.size,
			.blockCount   = MAX_FRAMES_IN_FLIGHT,
		};

		m_BufferInputs.emplace(HashStr(info.name.c_str()), new Buffer(bufferCreateInfo));
	}

	uint32_t passIndex = 0u;
	for (RenderPass::CreateInfo& renderPassCreateInfo : m_RenderPassCreateInfos)
	{
		RenderPass& pass = m_RenderPasses[passIndex++];

		for (RenderPass::CreateInfo::BufferInputInfo& info : renderPassCreateInfo.bufferInputInfos)
		{
			BufferCreateInfo bufferCreateInfo {
				.device       = m_Device,
				.commandPool  = m_CommandPool,
				.usage        = info.type == vk::DescriptorType::eUniformBuffer ? vk::BufferUsageFlagBits::eUniformBuffer :
				                                                                  vk::BufferUsageFlagBits::eStorageBuffer,
				.minBlockSize = info.size,
				.blockCount   = MAX_FRAMES_IN_FLIGHT,
			};

			pass.bufferInputs.emplace(HashStr(info.name.c_str()), new Buffer(bufferCreateInfo));
		}
	}
}

void RenderGraph::BuildDescriptorSets()
{
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (const RenderPass::CreateInfo::BufferInputInfo& info : m_BufferInputInfos)
		{
			// Determine bindings
			bindings.push_back(vk::DescriptorSetLayoutBinding {
			    info.binding,
			    info.type,
			    info.count,
			    info.stageMask,
			});
		}
		// Create descriptor set layout
		vk::DescriptorSetLayoutCreateInfo layoutCreateInfo {
			{}, // flags
			static_cast<uint32_t>(bindings.size()),
			bindings.data(),

		};
		m_DescriptorSetLayout = m_Device->logical.createDescriptorSetLayout(layoutCreateInfo);

		// Allocate descriptor sets
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1,
				&m_DescriptorSetLayout,
			};
			m_DescriptorSets.push_back(m_Device->logical.allocateDescriptorSets(allocInfo)[0]);

			std::string descriptorSetName = "render_graph DescriptorSet #" + std::to_string(i);
			m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
			    vk::ObjectType::eDescriptorSet,
			    (uint64_t)(VkDescriptorSet)m_DescriptorSets.back(),
			    descriptorSetName.c_str(),
			});
		}

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
			{},
			1ull,                   // setLayoutCount
			&m_DescriptorSetLayout, // pSetLayouts
		};

		m_PipelineLayout = m_Device->logical.createPipelineLayout(pipelineLayoutCreateInfo);
	}

	uint32_t passIndex = 0u;
	for (const RenderPass::CreateInfo& renderPassCreateInfo : m_RenderPassCreateInfos)
	{
		RenderPass& pass = m_RenderPasses[passIndex++];


		// Determine bindings
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (const RenderPass::CreateInfo::BufferInputInfo info : renderPassCreateInfo.bufferInputInfos)
		{
			bindings.push_back(vk::DescriptorSetLayoutBinding {
			    info.binding,
			    info.type,
			    info.count,
			    info.stageMask,
			});
		}
		for (const RenderPass::CreateInfo::TextureInputInfo info : renderPassCreateInfo.textureInputInfos)
		{
			bindings.push_back(vk::DescriptorSetLayoutBinding {
			    info.binding,
			    info.type,
			    info.count,
			    info.stageMask,
			});
		}

		// Create descriptor set layout
		vk::DescriptorSetLayoutCreateInfo layoutCreateInfo {
			{}, // flags
			static_cast<uint32_t>(bindings.size()),
			bindings.data(),

		};
		pass.descriptorSetLayout = m_Device->logical.createDescriptorSetLayout(layoutCreateInfo);

		// Allocate descriptor sets
		if (!renderPassCreateInfo.bufferInputInfos.empty() || !renderPassCreateInfo.textureInputInfos.empty())
		{
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vk::DescriptorSetAllocateInfo allocInfo {
					m_DescriptorPool,
					1u,
					&pass.descriptorSetLayout,
				};
				pass.descriptorSets.push_back(m_Device->logical.allocateDescriptorSets(allocInfo)[0]);

				std::string descriptorSetName = pass.name + " DescriptorSet #" + std::to_string(i);
				m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
				    vk::ObjectType::eDescriptorSet,
				    (uint64_t)(VkDescriptorSet)pass.descriptorSets.back(),
				    descriptorSetName.c_str(),
				});
			}
		}

		// Create pipeline layout
		std::array<vk::DescriptorSetLayout, 2> layouts {
			m_DescriptorSetLayout,
			pass.descriptorSetLayout
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
			{},
			2ull,           // setLayoutCount
			layouts.data(), // pSetLayouts
		};

		pass.pipelineLayout = m_Device->logical.createPipelineLayout(pipelineLayoutCreateInfo);
	}
}

void RenderGraph::WriteDescriptorSets()
{
	{
		std::vector<vk::DescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(m_BufferInputs.size() * MAX_FRAMES_IN_FLIGHT);
		for (const RenderPass::CreateInfo::BufferInputInfo info : m_BufferInputInfos)
		{
			std::vector<vk::WriteDescriptorSet> writes;

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				bufferInfos.push_back({
				    *(m_BufferInputs[HashStr(info.name.c_str())]->GetBuffer()),
				    m_BufferInputs[HashStr(info.name.c_str())]->GetBlockSize() * i,
				    m_BufferInputs[HashStr(info.name.c_str())]->GetBlockSize(),
				});


				for (uint32_t j = 0; j < info.count; j++)
				{
					writes.push_back(vk::WriteDescriptorSet {
					    m_DescriptorSets[i], // dstSet
					    info.binding,
					    j,
					    1u,
					    info.type,
					    nullptr,
					    &bufferInfos.back(),
					});
				}
			}

			m_Device->logical.updateDescriptorSets(
			    static_cast<uint32_t>(writes.size()),
			    writes.data(),
			    0u,
			    nullptr);

			m_Device->logical.waitIdle();
		}
	}


	uint32_t passIndex = 0u;
	for (const RenderPass::CreateInfo& renderPassCreateInfo : m_RenderPassCreateInfos)
	{
		RenderPass& pass = m_RenderPasses[passIndex++];

		std::vector<vk::WriteDescriptorSet> writes;

		std::vector<vk::DescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(pass.bufferInputs.size() * MAX_FRAMES_IN_FLIGHT);
		for (const RenderPass::CreateInfo::BufferInputInfo info : renderPassCreateInfo.bufferInputInfos)
		{
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				bufferInfos.push_back({
				    *(pass.bufferInputs[HashStr(info.name.c_str())]->GetBuffer()),
				    info.size * i,
				    info.size,
				});

				for (uint32_t j = 0; j < info.count; j++)
				{
					writes.push_back(vk::WriteDescriptorSet {
					    pass.descriptorSets[i], // dstSet
					    info.binding,
					    j,
					    1u,
					    info.type,
					    nullptr,
					    &bufferInfos.back(),
					});
				}
			}
		}
		for (const RenderPass::CreateInfo::TextureInputInfo info : renderPassCreateInfo.textureInputInfos)
		{
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				for (uint32_t j = 0; j < info.count; j++)
				{
					writes.push_back(vk::WriteDescriptorSet {
					    pass.descriptorSets[i], // dstSet
					    info.binding,
					    j,
					    1u,
					    info.type,
					    &info.defaultTexture->descriptorInfo,
					    nullptr,
					});
				}
			}
		}

		m_Device->logical.updateDescriptorSets(
		    static_cast<uint32_t>(writes.size()),
		    writes.data(),
		    0u,
		    nullptr);

		m_Device->logical.waitIdle();
	}
}

void RenderGraph::CreateAttachmentResource(const RenderPass::CreateInfo::AttachmentInfo& info, RenderGraph::AttachmentResourceContainer::Type type)
{
	m_SampleCount = info.samples;

	vk::ImageUsageFlags usage;
	vk::ImageAspectFlags aspectMask;
	if (info.format == m_Device->surfaceFormat.format)
	{
		usage      = vk::ImageUsageFlagBits::eColorAttachment;
		aspectMask = vk::ImageAspectFlagBits::eColor;
	}
	else if (info.format == m_Device->depthFormat)
	{
		usage      = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		aspectMask = vk::ImageAspectFlagBits::eDepth;
	}
	else
	{
		BVK_ASSERT(true, "Unsupported render attachment format: {}", string_VkFormat(static_cast<VkFormat>(info.format)));
	}

	// Determine image extent
	vk::Extent3D extent;
	switch (info.sizeType)
	{
	case RenderPass::CreateInfo::SizeType::eSwapchainRelative:
		extent = {
			.width  = static_cast<uint32_t>(m_Device->surfaceCapabilities.maxImageExtent.width * info.size.x),
			.height = static_cast<uint32_t>(m_Device->surfaceCapabilities.maxImageExtent.height * info.size.y),
			.depth  = 1,
		};
		break;
	case RenderPass::CreateInfo::SizeType::eRelative:
		BVK_ASSERT(true, "Unimplemented attachment size type: Relative");
		break;

	case RenderPass::CreateInfo::SizeType::eAbsolute:
		extent = {
			.width  = static_cast<uint32_t>(info.size.x),
			.height = static_cast<uint32_t>(info.size.y),
			.depth  = 1,
		};
		break;

	default:
		BVK_ASSERT(true, "Invalid attachment size type");
	};

	AttachmentResourceContainer resourceContainer = {
		.type        = type,
		.imageFormat = info.format,

		.extent           = extent,
		.size             = info.size,
		.sizeType         = info.sizeType,
		.relativeSizeName = info.sizeRelativeName,

		.sampleCount            = info.samples,
		.transientMSResolveMode = vk::ResolveModeFlagBits::eNone,
		.transientMSImage       = {},
		.transientMSImageView   = {},
		.lastWriteName          = info.name,
	};

	switch (type)
	{
	case AttachmentResourceContainer::Type::ePerImage:
	{
		for (uint32_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			resourceContainer.resources.push_back(AttachmentResource {
			    .srcAccessMask  = {},
			    .srcImageLayout = vk::ImageLayout::eUndefined,
			    .srcStageMask   = vk::PipelineStageFlagBits::eTopOfPipe,
			    .image          = m_SwapchainImages[i],
			    .imageView      = m_SwapchainImageViews[i],
			});
		}

		m_SwapchainResourceIndex = m_AttachmentResources.size();
		break;
	}
	case AttachmentResourceContainer::Type::eSingle:
	{
		vk::ImageCreateInfo imageCreateInfo {
			{},                 // flags
			vk::ImageType::e2D, // imageType
			info.format,        // format
			extent,             // extent
			1u,                 // mipLevels
			1u,                 // arrayLayers

			aspectMask & vk::ImageAspectFlagBits::eColor ? vk::SampleCountFlagBits::e1 : info.samples, // samples

			vk::ImageTiling::eOptimal,   // tiling
			usage,                       // usage
			vk::SharingMode::eExclusive, // sharingMode
			0u,                          // queueFamilyIndexCount
			nullptr,                     // pQueueFamilyIndices
			vk::ImageLayout::eUndefined, // initialLayout
		};

		vma::AllocationCreateInfo imageAllocInfo({}, vma::MemoryUsage::eGpuOnly, vk::MemoryPropertyFlagBits::eDeviceLocal);
		AllocatedImage image = m_Device->allocator.createImage(imageCreateInfo, imageAllocInfo);

		std::string imageName = info.name + " Image";
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)(vk::Image)image,
		    imageName.c_str(),
		});

		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                     // flags
			image,                  // image
			vk::ImageViewType::e2D, // viewType
			info.format,            // format

			/* components */
			vk::ComponentMapping {
			    // Don't swizzle the colors around...
			    vk::ComponentSwizzle::eIdentity, // r
			    vk::ComponentSwizzle::eIdentity, // g
			    vk::ComponentSwizzle::eIdentity, // b
			    vk::ComponentSwizzle::eIdentity, // a
			},

			/* subresourceRange */
			vk::ImageSubresourceRange {
			    aspectMask, // aspectMask
			    0u,         // baseMipLevel
			    1u,         // levelCount
			    0u,         // baseArrayLayer
			    1u,         // layerCount
			},
		};
		vk::ImageView imageView   = m_Device->logical.createImageView(imageViewCreateInfo, nullptr);
		std::string imageViewName = info.name + " ImageView";
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (uint64_t)(VkImageView)imageView,
		    imageViewName.c_str(),
		});

		resourceContainer.resources.push_back(AttachmentResource {
		    .srcAccessMask  = {},
		    .srcImageLayout = vk::ImageLayout::eUndefined,
		    .srcStageMask   = vk::PipelineStageFlagBits::eTopOfPipe,
		    .image          = image,
		    .imageView      = imageView,
		});

		break;
	}
	default:
		BVK_ASSERT(true, "Invalid attachment resource type");
	}

	if (static_cast<uint32_t>(info.samples) > 1 && aspectMask & vk::ImageAspectFlagBits::eColor)
	{
		vk::ImageCreateInfo imageCreateInfo {
			{},                        // flags
			vk::ImageType::e2D,        // imageType
			info.format,               // format
			extent,                    // extent
			1u,                        // mipLevels
			1u,                        // arrayLayers
			info.samples,              // samples
			vk::ImageTiling::eOptimal, // tiling

			usage | vk::ImageUsageFlagBits::eTransientAttachment, // usage

			vk::SharingMode::eExclusive, // sharingMode
			0u,                          // queueFamilyIndexCount
			nullptr,                     // pQueueFamilyIndices
			vk::ImageLayout::eUndefined, // initialLayout
		};

		vma::AllocationCreateInfo imageAllocInfo({}, vma::MemoryUsage::eGpuOnly, vk::MemoryPropertyFlagBits::eDeviceLocal);
		AllocatedImage image = m_Device->allocator.createImage(imageCreateInfo, imageAllocInfo);

		std::string imageName = info.name + " TransientMS Image";
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)(vk::Image)image,
		    imageName.c_str(),
		});


		vk::ImageViewCreateInfo imageViewCreateInfo {
			{},                     // flags
			image,                  // image
			vk::ImageViewType::e2D, // viewType
			info.format,            // format

			/* components */
			vk::ComponentMapping {
			    // Don't swizzle the colors around...
			    vk::ComponentSwizzle::eIdentity, // r
			    vk::ComponentSwizzle::eIdentity, // g
			    vk::ComponentSwizzle::eIdentity, // b
			    vk::ComponentSwizzle::eIdentity, // a
			},

			/* subresourceRange */
			vk::ImageSubresourceRange {
			    aspectMask, // aspectMask
			    0u,         // baseMipLevel
			    1u,         // levelCount
			    0u,         // baseArrayLayer
			    1u,         // layerCount
			},
		};
		vk::ImageView imageView   = m_Device->logical.createImageView(imageViewCreateInfo, nullptr);
		std::string imageViewName = info.name + " TransientMS ImageView";
		m_Device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (uint64_t)(VkImageView)imageView,
		    imageViewName.c_str(),
		});

		resourceContainer.transientMSImage       = image;
		resourceContainer.transientMSImageView   = imageView;
		resourceContainer.transientMSResolveMode = vk::ResolveModeFlagBits::eAverage;
	}

	m_AttachmentResources.push_back(resourceContainer);
}

void RenderGraph::BeginFrame(RenderContext context)
{
	m_OnBeginFrame(context);

	for (RenderPass& pass : m_RenderPasses)
	{
		pass.onBeginFrame(context);
	}
}

void RenderGraph::EndFrame(RenderContext context)
{
	m_OnUpdate(context);
	// @todo: Setup render barriers
	const auto primaryCmd = context.cmd;

	for (uint32_t i = 0; i < m_RenderPasses.size(); i++)
	{
		context.pass = &m_RenderPasses[i];
		context.pass->onUpdate(context);
	}


	primaryCmd.begin(vk::CommandBufferBeginInfo {});


	size_t i = 0;
	for (RenderPass& pass : m_RenderPasses)
	{
		vk::CommandBuffer passCmd = m_SecondaryCommandBuffers[(context.frameIndex * m_RenderPasses.size()) + i];

		std::vector<vk::Format> colorAttachmentFormats = {};
		vk::Format depthAttachmentFormat               = {};
		for (RenderPass::Attachment attachment : pass.attachments)
		{
			if (attachment.subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor)
			{
				colorAttachmentFormats.push_back(m_Device->surfaceFormat.format);
			}
			else
			{
				depthAttachmentFormat = m_Device->depthFormat;
			}
		}


		vk::CommandBufferInheritanceRenderingInfo inheritanceRenderingInfo {
			{},
			{},
			(uint32_t)colorAttachmentFormats.size(),
			colorAttachmentFormats.data(),
			depthAttachmentFormat,
			{},
			m_SampleCount,
			{}
		};

		vk::CommandBufferInheritanceInfo inheritanceInfo {
			{},
			{},
			{},
			{},
			{},
			{},

			&inheritanceRenderingInfo,
		};
		passCmd.begin(vk::CommandBufferBeginInfo {
		    vk::CommandBufferUsageFlagBits::eRenderPassContinue,
		    &inheritanceInfo,
		});

		passCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                           m_PipelineLayout,
		                           0u, 1ul, &m_DescriptorSets[context.frameIndex], 0ul, {});
		if (!pass.descriptorSets.empty())
		{
			passCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			                           pass.pipelineLayout,
			                           1ul, 1u, &pass.descriptorSets[context.frameIndex], 0ul, {});
		}
		context.cmd = passCmd;
		pass.onRender(context);
		passCmd.end();

		i++;
	}

	i = 0;
	for (RenderPass& pass : m_RenderPasses)
	{
		primaryCmd.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT({
		    pass.name.c_str(),
		    { 1.0, 1.0, 0.8, 1.0 },
		}));

		std::vector<vk::RenderingAttachmentInfo> renderingColorAttachmentInfos;
		vk::RenderingAttachmentInfo renderingDepthAttachmentInfo;
		for (RenderPass::Attachment attachment : pass.attachments)
		{
			AttachmentResourceContainer& resourceContainer = m_AttachmentResources[attachment.resourceIndex];

			AttachmentResource& resource = resourceContainer.GetResource(context.imageIndex, context.frameIndex);

			vk::ImageMemoryBarrier imageBarrier = {
				resource.srcAccessMask,       // srcAccessMask
				attachment.accessMask,        // dstAccessMask
				resource.srcImageLayout,      // oldLayout
				attachment.layout,            // newLayout
				m_Device->graphicsQueueIndex, // srcQueueFamilyIndex
				m_Device->graphicsQueueIndex, // dstQueueFamilyIndex
				resource.image,               // image
				attachment.subresourceRange,  // subresourceRange
				nullptr,                      // pNext
			};

			if ((resource.srcAccessMask != attachment.accessMask ||
			     resource.srcImageLayout != attachment.layout ||
			     resource.srcStageMask != attachment.stageMask) &&
			    attachment.subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor)
			{
				primaryCmd.pipelineBarrier(resource.srcStageMask,
				                           attachment.stageMask,
				                           {},
				                           {},
				                           {},
				                           imageBarrier);

				resource.srcAccessMask  = attachment.accessMask;
				resource.srcImageLayout = attachment.layout;
				resource.srcStageMask   = attachment.stageMask;
			}


			vk::RenderingAttachmentInfo renderingAttachmentInfo {};
			// Multi-sampled image
			if (static_cast<uint32_t>(resourceContainer.sampleCount) > 1 && attachment.subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor)
			{
				renderingAttachmentInfo = {
					resourceContainer.transientMSImageView,
					attachment.layout,

					resourceContainer.transientMSResolveMode,
					resource.imageView,
					attachment.layout,

					attachment.loadOp,
					attachment.storeOp,

					attachment.clearValue,
				};
			}
			// Single-sampled image
			else
			{
				renderingAttachmentInfo = {
					resource.imageView,
					attachment.layout,

					vk::ResolveModeFlagBits::eNone,
					{},
					{},

					attachment.loadOp,
					attachment.storeOp,

					attachment.clearValue,
				};
			}

			if (attachment.subresourceRange.aspectMask & vk::ImageAspectFlagBits::eDepth)
			{
				renderingDepthAttachmentInfo = renderingAttachmentInfo;
			}
			else
			{
				renderingColorAttachmentInfos.push_back(renderingAttachmentInfo);
			}
		}


		vk::RenderingInfo renderingInfo {
			vk::RenderingFlagBits::eContentsSecondaryCommandBuffers, // flags
			vk::Rect2D {
			    { 0, 0 },
			    m_Device->surfaceCapabilities.maxImageExtent,
			},
			1u,
			{},
			static_cast<uint32_t>(renderingColorAttachmentInfos.size()),
			renderingColorAttachmentInfos.data(),
			renderingDepthAttachmentInfo.imageView ? &renderingDepthAttachmentInfo : nullptr,
			{},
		};

		primaryCmd.beginRendering(renderingInfo);


		primaryCmd.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT({
		    (pass.name + " render action").c_str(),
		    { 1.0, 1.0, 0.2, 1.0 },
		}));

		primaryCmd.executeCommands(m_SecondaryCommandBuffers[(context.frameIndex * m_RenderPasses.size()) + i]);
		primaryCmd.endRendering();

		primaryCmd.endDebugUtilsLabelEXT();
		primaryCmd.endDebugUtilsLabelEXT();
		i++;
	}

	AttachmentResource& backbufferResource = m_AttachmentResources[m_SwapchainResourceIndex].GetResource(context.imageIndex, context.frameIndex);

	// Transition color image to present optimal
	vk::ImageMemoryBarrier imageMemoryBarrier = {
		backbufferResource.srcAccessMask,  // srcAccessMask
		{},                                // dstAccessMask
		backbufferResource.srcImageLayout, // oldLayout
		vk::ImageLayout::ePresentSrcKHR,   // newLayout
		{},
		{},
		backbufferResource.image, // image

		/* subresourceRange */
		vk::ImageSubresourceRange {
		    vk::ImageAspectFlagBits::eColor,
		    0u,
		    1u,
		    0u,
		    1u,
		},
	};

	primaryCmd.pipelineBarrier(
	    backbufferResource.srcStageMask,
	    vk::PipelineStageFlagBits::eBottomOfPipe,
	    {},
	    {},
	    {},
	    imageMemoryBarrier);

	backbufferResource.srcStageMask   = vk::PipelineStageFlagBits::eTopOfPipe;
	backbufferResource.srcImageLayout = vk::ImageLayout::eUndefined;
	backbufferResource.srcAccessMask  = {};
}

} // namespace BINDLESSVK_NAMESPACE
