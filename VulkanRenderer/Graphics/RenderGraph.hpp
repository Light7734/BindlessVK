#pragma once

#include <vulkan/vulkan_structs.hpp>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Scene/Camera.hpp"
// #include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif
// @wip finish this....
class RenderGraph
{
public:
	struct CreateInfo
	{
		uint32_t swapchainImageCount;
		vk::Extent2D swapchainExtent;
		vk::DescriptorPool descriptorPool;
		vk::Device logicalDevice;
		vk::PhysicalDevice physicalDevice;
		vma::Allocator allocator;
		vk::CommandPool commandPool;
		QueueInfo queueInfo;
	};

	struct UpdateData
	{
		RenderGraph& graph;
		uint32_t frameIndex;
		vk::Device logicalDevice;
		Scene* scene;
	};

public:
	RenderGraph()
	{
	}

	~RenderGraph()
	{
	}

	void Init(const CreateInfo& info)
	{
		m_SwapchainImageCount = info.swapchainImageCount;
		m_SwapchainExtent     = info.swapchainExtent;
		m_LogicalDevice       = info.logicalDevice;
		m_DescriptorPool      = info.descriptorPool;
		m_PhysicalDevice      = info.physicalDevice;
		m_Allocator           = info.allocator;
		m_CommandPool         = info.commandPool;
		m_QueueInfo           = info.queueInfo;
	}


	void ResolveGraph()
	{
		std::vector<vk::DescriptorSetLayoutBinding> graphDescriptorSetLayoutBindings = {};
		uint32_t graphDescriptorSetIndex                                             = 0u;
		for (RenderPass::DescriptorInfo descriptorInfo : m_DescriptorInfos)
		{
			graphDescriptorSetLayoutBindings.push_back({
			    graphDescriptorSetIndex++,
			    descriptorInfo.type,
			    descriptorInfo.count,
			    descriptorInfo.stageMask,
			    nullptr,
			});

			// @todo: Add support for other buffer types
			if (descriptorInfo.type == vk::DescriptorType::eUniformBuffer || descriptorInfo.type == vk::DescriptorType::eStorageBuffer)
			{
				for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
				{
					BufferCreateInfo createInfo {
						m_LogicalDevice,           // logicalDevice
						m_PhysicalDevice,          // physicalDevice
						m_Allocator,               // vmaallocator
						m_CommandPool,             // commandPool
						m_QueueInfo.graphicsQueue, // graphicsQueue

						// usage
						descriptorInfo.type == vk::DescriptorType::eUniformBuffer ? vk::BufferUsageFlagBits::eUniformBuffer :
						                                                            vk::BufferUsageFlagBits::eStorageBuffer,

						descriptorInfo.bufferSize,  // size
						descriptorInfo.initialData, // inital data
					};

					LOG(warn, "Creating descriptor buffer {}-{}", descriptorInfo.name, i);
					m_DescriptorBuffers[HashStr(descriptorInfo.name.c_str())][i] = new Buffer(createInfo);
				}
			}
		};

		vk::DescriptorSetLayoutCreateInfo graphDescriptorSetLayoutCreateInfo {
			{},
			static_cast<uint32_t>(graphDescriptorSetLayoutBindings.size()),
			graphDescriptorSetLayoutBindings.data(),
		};
		m_DescriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(graphDescriptorSetLayoutCreateInfo, nullptr);

		vk::PipelineLayoutCreateInfo graphPipelineLayoutCreateInfo {
			{}, // flags
			1u,
			&m_DescriptorSetLayout,
		};
		m_PipelineLayout = m_LogicalDevice.createPipelineLayout(graphPipelineLayoutCreateInfo);


		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorSetAllocateInfo allocInfo {
				m_DescriptorPool,
				1ul,
				&m_DescriptorSetLayout,
			};

			// SET descriptor SET...
			m_DescriptorSets.push_back(m_LogicalDevice.allocateDescriptorSets(allocInfo)[0]);
		}

		/////////////////////////////////////////////////////////////////////////////////
		/// Descriptors
		for (RenderPass* renderPass : m_RenderPasses)
		{
			std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {};

			uint32_t i = 0;
			for (RenderPass::DescriptorInfo descriptorInfo : renderPass->GetDescriptorInfos())
			{
				descriptorSetLayoutBindings.push_back({
				    i++,
				    descriptorInfo.type,
				    descriptorInfo.count,
				    descriptorInfo.stageMask,
				    nullptr,
				});

				// @todo: Add support for other buffer types
				if (descriptorInfo.type == vk::DescriptorType::eUniformBuffer || descriptorInfo.type == vk::DescriptorType::eStorageBuffer)
				{
					for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
					{
						BufferCreateInfo createInfo {
							m_LogicalDevice,           // logicalDevice
							m_PhysicalDevice,          // physicalDevice
							m_Allocator,               // vmaallocator
							m_CommandPool,             // commandPool
							m_QueueInfo.graphicsQueue, // graphicsQueue

							// usage
							descriptorInfo.type == vk::DescriptorType::eUniformBuffer ? vk::BufferUsageFlagBits::eUniformBuffer :
							                                                            vk::BufferUsageFlagBits::eStorageBuffer,

							descriptorInfo.bufferSize,  // size
							descriptorInfo.initialData, // inital data
						};

						renderPass->SetDescriptorResource(descriptorInfo.name.c_str(), i, new Buffer(createInfo));
					}
				}
			}

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
				{},
				static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
				descriptorSetLayoutBindings.data(),
			};

			vk::DescriptorSetLayout descriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo, nullptr);
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vk::DescriptorSetAllocateInfo allocInfo {
					m_DescriptorPool,
					1ul,
					&descriptorSetLayout,
				};

				// SET descriptor SET...
				renderPass->SetDescriptorSet(m_LogicalDevice.allocateDescriptorSets(allocInfo)[0], i);
			}

			std::array<vk::DescriptorSetLayout, 2u> pipelineDescriptorSetLayouts {
				m_DescriptorSetLayout,
				descriptorSetLayout,
			};

			vk::PipelineLayoutCreateInfo renderPassPipelineLayoutCreateInfo {
				{}, // flags
				static_cast<uint32_t>(pipelineDescriptorSetLayouts.size()),
				pipelineDescriptorSetLayouts.data(),
			};
			renderPass->SetPipelineLayout(m_LogicalDevice.createPipelineLayout(renderPassPipelineLayoutCreateInfo));
		}


		/////////////////////////////////////////////////////////////////////////////////
		// Barriers
		// @todo: Create a top level barrier
		size_t i = 1;
		std::unordered_map<uint64_t, RenderPass::AttachmentInfo> lastStates;

		m_ImageBarries.resize(m_RenderPasses.size() + 1);
		for (RenderPass* renderPass : m_RenderPasses)
		{
			std::vector<ImageBarrier>& barriers = m_ImageBarries[i++];
			barriers                            = {};

			for (RenderPass::AttachmentInfo attachment : renderPass->GetAttachments())
			{
				uint64_t key = HashStr(attachment.name.c_str());

				// an image barrier is already applied
				if (lastStates.contains(key))
				{
					RenderPass::AttachmentInfo lastState;

					vk::PipelineStageFlags srcStageMask = lastState.stageMask;
					vk::PipelineStageFlags dstStageMask = attachment.stageMask;

					vk::AccessFlags srcAccessMask = lastState.accessMask;
					vk::AccessFlags dstAccessMask = attachment.accessMask;

					vk::ImageLayout srcLayout = lastState.layout;
					vk::ImageLayout dstLayout = attachment.layout;

					if (srcStageMask != dstStageMask ||
					    srcAccessMask != dstAccessMask ||
					    srcLayout != dstLayout)
					{
						if (attachment.images.size() == 1u)
						{
							barriers.push_back({
							    .srcStageMask  = srcStageMask,
							    .dstStageMask  = dstStageMask,
							    .barrierObject = {
							        srcAccessMask,
							        dstAccessMask,
							        srcLayout,
							        dstLayout,
							        {},
							        {},
							        attachment.images[0],

							        // @todo: don't assumbe color
							        vk::ImageSubresourceRange {
							            vk::ImageAspectFlagBits::eColor,
							            0u,
							            1u,
							            0u,
							            1u,
							        },
							    },
							});
						}
						else if (attachment.images.size() == m_SwapchainImageCount)
						{
							for (uint32_t j = 0; j < m_SwapchainImageCount; j++)
							{
								barriers.push_back({
								    .srcStageMask  = srcStageMask,
								    .dstStageMask  = dstStageMask,
								    .barrierObject = {
								        srcAccessMask,
								        dstAccessMask,
								        srcLayout,
								        dstLayout,
								        {},
								        {},
								        attachment.images[j],

								        // @todo: don't assumbe color
								        vk::ImageSubresourceRange {
								            vk::ImageAspectFlagBits::eColor,
								            0u,
								            1u,
								            0u,
								            1u,
								        },
								    },
								    .imageIndex = j,
								});
							}
						}
						else
						{
							ASSERT(false, "@TODO");
						}
					}
					else
					{
						LOG(info, "Found matching barriers, skippin...");
					}
				}

				// image is in default layout with no active barriers
				else
				{
					vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
					vk::PipelineStageFlags dstStageMask = attachment.stageMask;

					vk::AccessFlags srcAccessMask = {};
					vk::AccessFlags dstAccessMask = attachment.accessMask;

					vk::ImageLayout srcLayout = vk::ImageLayout::eUndefined;
					vk::ImageLayout dstLayout = attachment.layout;

					if (attachment.images.size() == 1u)
					{
						barriers.push_back({
						    .srcStageMask  = srcStageMask,
						    .dstStageMask  = dstStageMask,
						    .barrierObject = {
						        srcAccessMask,
						        dstAccessMask,
						        srcLayout,
						        dstLayout,
						        {},
						        {},
						        attachment.images[0],

						        // @todo: don't assumbe color
						        vk::ImageSubresourceRange {
						            vk::ImageAspectFlagBits::eColor,
						            0u,
						            1u,
						            0u,
						            1u,
						        },
						    },
						});
					}

					else if (attachment.images.size() == m_SwapchainImageCount)
					{
						for (uint32_t j = 0; j < m_SwapchainImageCount; j++)
						{
							barriers.push_back({
							    .srcStageMask  = srcStageMask,
							    .dstStageMask  = dstStageMask,
							    .barrierObject = {
							        srcAccessMask,
							        dstAccessMask,
							        srcLayout,
							        dstLayout,
							        {},
							        {},
							        attachment.images[j],

							        // @todo: don't assumbe color
							        vk::ImageSubresourceRange {
							            vk::ImageAspectFlagBits::eColor,
							            0u,
							            1u,
							            0u,
							            1u,
							        },
							    },
							    .imageIndex = j,
							});
						}
					}
				}

				lastStates[key] = attachment;
			}
		}


		/////////////////////////////////////////////////////////////////////////////////
		/// RenderingInfos
		for (RenderPass* renderPass : m_RenderPasses)
		{
			m_RenderingInfos.push_back({});
			std::vector<vk::RenderingInfo>& infos = m_RenderingInfos.back();
			for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
			{
				std::vector<vk::RenderingAttachmentInfo> colorInfos;
				vk::RenderingAttachmentInfo depthInfo = {};

				for (RenderPass::AttachmentInfo attachment : renderPass->GetAttachments())
				{
					if (attachment.layout == vk::ImageLayout::eColorAttachmentOptimal)
					{
						vk::RenderingAttachmentInfo abc {
							attachment.views[0],
							attachment.layout,

							attachment.resolveMode,
							attachment.resolveView[i],
							attachment.resolveLayout,

							attachment.loadOp,
							attachment.storeOp,

							/* clearValue */
							vk::ClearColorValue {
							    std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f },
							},
						};
					}
					else if (attachment.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
					{
						depthInfo = {
							attachment.views[0],
							attachment.layout,
							{},
							{},
							{},
							attachment.loadOp,
							attachment.storeOp,

							/* clearValue */
							vk::ClearDepthStencilValue {
							    { 1.0, 0 },
							},
						};
					}
				}

				infos.push_back({
				    {},

				    vk::Rect2D {
				        { 0, 0 },
				        m_SwapchainExtent,
				    },

				    1u,
				    {},
				    static_cast<uint32_t>(colorInfos.size()),
				    colorInfos.data(),
				    &depthInfo,
				    {},
				});
			}
		}
	}

	RenderGraph& SetName(const std::string& name)
	{
		m_Name = name;
		return *this;
	}

	RenderGraph& AddDescriptor(const RenderPass::DescriptorInfo& info)
	{
		m_DescriptorInfos.push_back(info);
		return *this;
	}

	RenderGraph& AddRenderPass(RenderPass* pass)
	{
		m_RenderPasses.push_back(pass);
		return *this;
	}

	RenderGraph& SetUpdateAction(std::function<void(const RenderGraph::UpdateData&)> updateAction)
	{
		m_UpdateAction = updateAction;
		return *this;
	}

	void Update(Scene* scene, uint32_t frameIndex)
	{
		m_UpdateAction({
		    .graph         = *this,
		    .frameIndex    = frameIndex,
		    .logicalDevice = m_LogicalDevice,
		    .scene         = scene,
		});

		for (auto* renderPass : m_RenderPasses)
		{
			LOG(critical, "{}", renderPass->GetName());
			renderPass->Update({
			    .renderPass    = *renderPass,
			    .frameIndex    = frameIndex,
			    .logicalDevice = m_LogicalDevice,
			    .scene         = scene,
			});
		}
	}

	void Render(const RenderPass::RenderData& data)
	{
		// @todo: Setup render barriers
		const auto cmd = data.cmd;

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                       m_PipelineLayout,
		                       0u, m_DescriptorSets[data.frameIndex], 0u);

		size_t i = 1;
		for (auto* renderPass : m_RenderPasses)
		{
			for (ImageBarrier& barrier : m_ImageBarries[i])
			{
				if (barrier.imageIndex != UINT32_MAX && barrier.imageIndex != data.imageIndex)
					continue;

				cmd.pipelineBarrier(
				    barrier.srcStageMask,
				    barrier.dstStageMask,
				    {},
				    {},
				    {},
				    barrier.barrierObject);
			}

			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			                       renderPass->GetPipelineLayout(),
			                       1u, renderPass->GetDescriptorSet(data.frameIndex), 0u);


			cmd.beginRendering(m_RenderingInfos[i][data.imageIndex]);
			renderPass->Render(data);
			cmd.endRendering();
			i++;
		}

		// @wip: Setup present barriers
	}

	Buffer* GetDescriptorBuffer(const char* name, uint32_t frameIndex)
	{
		return m_DescriptorBuffers[HashStr(name)][frameIndex];
	}


private:
	struct ImageBarrier
	{
		vk::PipelineStageFlags srcStageMask;
		vk::PipelineStageFlags dstStageMask;
		vk::ImageMemoryBarrier barrierObject;


		uint32_t imageIndex = UINT32_MAX;
	};

	std::string m_Name = {};

	std::function<void(const RenderGraph::UpdateData&)> m_UpdateAction = {};

	std::vector<RenderPass*> m_RenderPasses                      = {};
	std::vector<std::vector<ImageBarrier>> m_ImageBarries        = {};
	std::vector<std::vector<vk::RenderingInfo>> m_RenderingInfos = {};

	std::unordered_map<uint64_t, std::array<Buffer*, MAX_FRAMES_IN_FLIGHT>> m_DescriptorBuffers = {};

	vma::Allocator m_Allocator = {};

	std::vector<vk::DescriptorSet> m_DescriptorSets;

	std::vector<RenderPass::DescriptorInfo> m_DescriptorInfos = {};

	uint32_t m_SwapchainImageCount      = {};
	vk::Extent2D m_SwapchainExtent      = {};
	vk::DescriptorPool m_DescriptorPool = {};


	vk::PipelineLayout m_PipelineLayout;
	vk::DescriptorSetLayout m_DescriptorSetLayout;

	vk::Device m_LogicalDevice;
	vk::PhysicalDevice m_PhysicalDevice;
	vk::CommandPool m_CommandPool;
	QueueInfo m_QueueInfo;
};
