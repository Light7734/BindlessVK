#pragma once

#include "BindlessVk/Device.hpp"
#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Renderer.hpp"
#include "Framework/Core/Application.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/Timer.hpp"

// passes
#include "ForwardPass.hpp"
#include "Framework/Core/Application.hpp"
#include "Graph.hpp"
#include "UserInterfacePass.hpp"

class DevelopmentExampleApplication: public Application
{
public:
	DevelopmentExampleApplication()
	{
		LoadShaders();
		LoadShaderEffects();
		LoadShaderPasses();
		LoadMasterMaterials();
		LoadMaterials();
		LoadModels();
		LoadEntities();
		CreateRenderGraph();
	}

	~DevelopmentExampleApplication()
	{
	}

	virtual void OnVulkanReady() override
	{
	}

	virtual void OnTick(double deltaTime) override
	{
		m_Renderer.BeginFrame(&m_Scene);

		ImGui::ShowDemoWindow();
		CVar::DrawImguiEditor();

        m_CameraController.Update();

		m_Renderer.EndFrame(&m_Scene);
	}

	virtual void OnSwapchainRecreate() override
	{
		ASSERT(false, "Swapchain recreation not supported");
	}

private:
	void LoadShaders()
	{
		const char* const DIRECTORY          = "Shaders/";
		const char* const VERTEX_EXTENSION   = ".spv_vs";
		const char* const FRAGMENT_EXTENSION = ".spv_fs";

		for (auto& shader : std::filesystem::directory_iterator(DIRECTORY))
		{
			std::string path(shader.path().c_str());
			std::string extension(shader.path().extension().c_str());
			std::string name(shader.path().filename().replace_extension().c_str());

			if (strcmp(extension.c_str(), VERTEX_EXTENSION) &&
			    strcmp(extension.c_str(), FRAGMENT_EXTENSION))
			{
				continue;
			}

			m_MaterialSystem.LoadShader({
			    name.c_str(),
			    path.c_str(),
			    !strcmp(extension.c_str(), VERTEX_EXTENSION) ? vk::ShaderStageFlagBits::eVertex : vk::ShaderStageFlagBits::eFragment,
			});
		}
	}

	void LoadShaderEffects()
	{
		// @TODO: Load from files instead of hard-coding
		m_MaterialSystem.CreateShaderEffect({
		    "default",
		    {
		        m_MaterialSystem.GetShader("defaultVertex"),
		        m_MaterialSystem.GetShader("defaultFragment"),
		    },
		});

		m_MaterialSystem.CreateShaderEffect({
		    "skybox",
		    {
		        m_MaterialSystem.GetShader("skybox_vertex"),
		        m_MaterialSystem.GetShader("skybox_fragment"),
		    },
		});
	}

	void LoadShaderPasses()
	{
		bvk::Device* device = m_DeviceSystem.GetDevice();

		const vk::Extent2D extent                 = device->surfaceCapabilities.maxImageExtent;
		const vk::SampleCountFlagBits sampleCount = device->maxDepthColorSamples;
		const vk::Format colorAttachmentFormat    = device->surfaceFormat.format;
		const vk::Format depthAttachmentFormat    = device->depthFormat;

		{
			// @TODO: Load from files instead of hard-coding
			std::vector<vk::VertexInputBindingDescription> inputBindings {
				vk::VertexInputBindingDescription {
				    0u,                                                // binding
				    static_cast<uint32_t>(sizeof(bvk::Model::Vertex)), // stride
				    vk::VertexInputRate::eVertex,                      // inputRate
				},
			};
			std::vector<vk::VertexInputAttributeDescription> inputAttributes {
				vk::VertexInputAttributeDescription {
				    0u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    0u,                           // offset
				},
				vk::VertexInputAttributeDescription {
				    1u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    sizeof(glm::vec3),            // offset
				},
				vk::VertexInputAttributeDescription {
				    2u,                                    // location
				    0u,                                    // binding
				    vk::Format::eR32G32B32Sfloat,          // format
				    sizeof(glm::vec3) + sizeof(glm::vec3), // offset
				},
				vk::VertexInputAttributeDescription {
				    3u,                                                        // location
				    0u,                                                        // binding
				    vk::Format::eR32G32Sfloat,                                 // format
				    sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec3), // offset
				},
				vk::VertexInputAttributeDescription {
				    4u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) +
				        sizeof(glm::vec3), // offset
				},
			};

			// Viewport state
			vk::Viewport viewport {
				0.0f,                              // x
				0.0f,                              // y
				static_cast<float>(extent.width),  // width
				static_cast<float>(extent.height), // height
				0.0f,                              // minDepth
				1.0f,                              // maxDepth
			};

			vk::Rect2D scissors {
				{ 0, 0 }, // offset
				extent    // extent
			};

			vk::PipelineColorBlendAttachmentState colorBlendAttachment {
				VK_FALSE,                           // blendEnable
				vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
				vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
				vk::BlendOp::eAdd,                  // colorBlendOp
				vk::BlendFactor::eOne,              // srcAlphaBlendFactor
				vk::BlendFactor::eZero,             // dstAlphaBlendFactor
				vk::BlendOp::eAdd,                  // alphaBlendOp

				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				    vk::ColorComponentFlagBits::eB |
				    vk::ColorComponentFlagBits::eA, // colorWriteMask
			};

			// create shader passes
			m_MaterialSystem.CreateShaderPass({
			    "default",
			    {
			        vk::PipelineVertexInputStateCreateInfo {
			            {},
			            static_cast<uint32_t>(inputBindings.size()),
			            inputBindings.data(),
			            static_cast<uint32_t>(inputAttributes.size()),
			            inputAttributes.data(),
			        },
			        vk::PipelineInputAssemblyStateCreateInfo {
			            {},
			            vk::PrimitiveTopology::eTriangleList,
			            VK_FALSE,
			        },
			        vk::PipelineTessellationStateCreateInfo {},
			        vk::PipelineViewportStateCreateInfo {
			            {},
			            1u,
			            &viewport,
			            1u,
			            &scissors,
			        },
			        vk::PipelineRasterizationStateCreateInfo {
			            {},
			            VK_FALSE,
			            VK_FALSE,
			            vk::PolygonMode::eFill,
			            vk::CullModeFlagBits::eBack,
			            vk::FrontFace::eClockwise,
			            VK_FALSE,
			            0.0f,
			            0.0f,
			            0.0f,
			            1.0f,
			        },
			        vk::PipelineMultisampleStateCreateInfo {
			            {},
			            sampleCount,
			            VK_FALSE,
			            {},
			            VK_FALSE,
			            VK_FALSE,
			        },
			        vk::PipelineDepthStencilStateCreateInfo {
			            {},
			            VK_TRUE,              // depthTestEnable
			            VK_TRUE,              // depthWriteEnable
			            vk::CompareOp::eLess, // depthCompareOp
			            VK_FALSE,             // depthBoundsTestEnable
			            VK_FALSE,             // stencilTestEnable
			            {},                   // front
			            {},                   // back
			            0.0f,                 // minDepthBounds
			            1.0,                  // maxDepthBounds
			        },
			        vk::PipelineColorBlendStateCreateInfo {
			            {},                         // flags
			            VK_FALSE,                   // logicOpEnable
			            vk::LogicOp::eCopy,         // logicOp
			            1u,                         // attachmentCount
			            &colorBlendAttachment,      // pAttachments
			            { 0.0f, 0.0f, 0.0f, 0.0f }, // blendConstants
			        },
			        vk::PipelineDynamicStateCreateInfo {},
			    },
			    m_MaterialSystem.GetShaderEffect("default"),
			    colorAttachmentFormat,
			    depthAttachmentFormat,
			});
		}

		{
			std::vector<vk::VertexInputBindingDescription> inputBindings {
				vk::VertexInputBindingDescription {
				    0u,                                                // binding
				    static_cast<uint32_t>(sizeof(bvk::Model::Vertex)), // stride
				    vk::VertexInputRate::eVertex,                      // inputRate
				},
			};
			std::vector<vk::VertexInputAttributeDescription> inputAttributes {
				vk::VertexInputAttributeDescription {
				    0u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    0u,                           // offset
				},
				vk::VertexInputAttributeDescription {
				    1u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    sizeof(glm::vec3),            // offset
				},
				vk::VertexInputAttributeDescription {
				    2u,                                    // location
				    0u,                                    // binding
				    vk::Format::eR32G32B32Sfloat,          // format
				    sizeof(glm::vec3) + sizeof(glm::vec3), // offset
				},
				vk::VertexInputAttributeDescription {
				    3u,                                                        // location
				    0u,                                                        // binding
				    vk::Format::eR32G32Sfloat,                                 // format
				    sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec3), // offset
				},
				vk::VertexInputAttributeDescription {
				    4u,                           // location
				    0u,                           // binding
				    vk::Format::eR32G32B32Sfloat, // format
				    sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) +
				        sizeof(glm::vec3), // offset
				},
			};
			// Viewport state
			vk::Viewport viewport {
				0.0f,                              // x
				0.0f,                              // y
				static_cast<float>(extent.width),  // width
				static_cast<float>(extent.height), // height
				0.0f,                              // minDepth
				1.0f,                              // maxDepth
			};

			vk::Rect2D scissors {
				{ 0, 0 }, // offset
				extent    // extent
			};

			vk::PipelineColorBlendAttachmentState colorBlendAttachment {
				VK_FALSE,                           // blendEnable
				vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
				vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
				vk::BlendOp::eAdd,                  // colorBlendOp
				vk::BlendFactor::eOne,              // srcAlphaBlendFactor
				vk::BlendFactor::eZero,             // dstAlphaBlendFactor
				vk::BlendOp::eAdd,                  // alphaBlendOp

				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				    vk::ColorComponentFlagBits::eB |
				    vk::ColorComponentFlagBits::eA, // colorWriteMask
			};

			// create shader passes
			m_MaterialSystem.CreateShaderPass({
			    "skybox",
			    {

			        vk::PipelineVertexInputStateCreateInfo {
			            {},
			            static_cast<uint32_t>(inputBindings.size()),
			            inputBindings.data(),
			            static_cast<uint32_t>(inputAttributes.size()),
			            inputAttributes.data(),
			        },

			        vk::PipelineInputAssemblyStateCreateInfo {
			            {},
			            vk::PrimitiveTopology::eTriangleList,
			            VK_FALSE,
			        },

			        vk::PipelineTessellationStateCreateInfo {},

			        vk::PipelineViewportStateCreateInfo {
			            {},
			            1u,
			            &viewport,
			            1u,
			            &scissors,
			        },

			        vk::PipelineRasterizationStateCreateInfo {
			            {},
			            VK_FALSE,
			            VK_FALSE,
			            vk::PolygonMode::eFill,
			            vk::CullModeFlagBits::eBack,
			            vk::FrontFace::eCounterClockwise,
			            VK_FALSE,
			            0.0f,
			            0.0f,
			            0.0f,
			            1.0f,
			        },
			        vk::PipelineMultisampleStateCreateInfo {
			            {},
			            sampleCount,
			            VK_FALSE,
			            {},
			            VK_FALSE,
			            VK_FALSE,
			        },

			        vk::PipelineDepthStencilStateCreateInfo {
			            {},
			            VK_TRUE,                     // depthTestEnable
			            VK_TRUE,                     // depthWriteEnable
			            vk::CompareOp::eLessOrEqual, // depthCompareOp
			            VK_FALSE,                    // depthBoundsTestEnable
			            VK_FALSE,                    // stencilTestEnable
			            {},                          // front
			            {},                          // back
			            0.0f,                        // minDepthBounds
			            1.0,                         // maxDepthBounds
			        },

			        vk::PipelineColorBlendStateCreateInfo {
			            {},                         // flags
			            VK_FALSE,                   // logicOpEnable
			            vk::LogicOp::eCopy,         // logicOp
			            1u,                         // attachmentCount
			            &colorBlendAttachment,      // pAttachments
			            { 0.0f, 0.0f, 0.0f, 0.0f }, // blendConstants
			        },
			        vk::PipelineDynamicStateCreateInfo {},
			    },

			    m_MaterialSystem.GetShaderEffect("skybox"),
			    colorAttachmentFormat,
			    depthAttachmentFormat,
			});
		}
	}

	void LoadMasterMaterials()
	{
		// @TODO: Load from files instead of hard-coding
		m_MaterialSystem.CreateMasterMaterial({
		    "default",
		    m_MaterialSystem.GetShaderPass("default"),
		    {},
		});

		m_MaterialSystem.CreateMasterMaterial({
		    "skybox",
		    m_MaterialSystem.GetShaderPass("skybox"),
		    {},
		});
	}

	void LoadMaterials()
	{
		// @TODO: Load from files instead of hard-coding
		m_MaterialSystem.CreateMaterial({
		    "default",
		    m_MaterialSystem.GetMasterMaterial("default"),
		    {},
		    {},
		});

		m_MaterialSystem.CreateMaterial({
		    "skybox",
		    m_MaterialSystem.GetMasterMaterial("skybox"),
		    {},
		    {},
		});
	}

	void LoadModels()
	{
		// @TODO: Load from files instead of hard-coding
		m_ModelSystem.LoadModel({
		    m_TextureSystem,
		    "default",
		    "Assets/FlightHelmet/FlightHelmet.gltf",
		});

		m_ModelSystem.LoadModel({
		    m_TextureSystem,
		    "skybox",
		    "Assets/Cube/Cube.gltf",
		});
	}

	void LoadEntities()
	{
		// @TODO: Load from files instead of hard-coding

		Entity testModel = m_Scene.create();

		m_Scene.emplace<TransformComponent>(testModel,
		                                    glm::vec3(0.0f),          // Translation
		                                    glm::vec3(1.0f),          // Scale
		                                    glm::vec3(0.0f, 0.0, 0.0) // Rotation
		);

		m_Scene.emplace<StaticMeshRendererComponent>(
		    testModel,
		    m_MaterialSystem.GetMaterial("default"), // Material
		    m_ModelSystem.GetModel("default")        // Mesh
		);

		Entity skybox = m_Scene.create();
		m_Scene.emplace<TransformComponent>(skybox,
		                                    glm::vec3(0.0f),          // Translation
		                                    glm::vec3(1.0f),          // Scale
		                                    glm::vec3(0.0f, 0.0, 0.0) // Rotation
		);

		m_Scene.emplace<StaticMeshRendererComponent>(
		    skybox,
		    m_MaterialSystem.GetMaterial("skybox"), // Material
		    m_ModelSystem.GetModel("skybox")        // Mesh
		);

		Entity light = m_Scene.create();
		m_Scene.emplace<TransformComponent>(
		    light, glm::vec3(2.0f, 2.0f, 1.0f), // Translation
		    glm::vec3(1.0f),                    // Scale
		    glm::vec3(0.0f, 0.0, 0.0)           // Rotation
		);

		Entity camera = m_Scene.create();
		m_Scene.emplace<TransformComponent>(camera, glm::vec3(6.0, 7.0, 2.5),
		                                    glm::vec3(1.0),
		                                    glm::vec3(0.0f, 0.0, 0.0));
		m_Scene.emplace<CameraComponent>(camera, 45.0f, 5.0, 1.0, 0.001f, 100.0f,
		                                 225.0, 0.0, glm::vec3(0.0f, 0.0f, -1.0f),
		                                 glm::vec3(0.0f, -1.0f, 0.0f), 10.0f);

		m_Scene.emplace<LightComponent>(light, 12);
	}

	void CreateRenderGraph()
	{
		bvk::Device* device                       = m_DeviceSystem.GetDevice();
		const vk::Format colorFormat              = device->surfaceFormat.format;
		const vk::Format deppthFormat             = device->depthFormat;
		const vk::SampleCountFlagBits sampleCount = device->maxDepthColorSamples;

		bvk::Texture* defaultTexture     = m_TextureSystem.GetTexture("default");
		bvk::Texture* defaultTextureCube = m_TextureSystem.GetTexture("default_cube");

		bvk::RenderPass::CreateInfo forwardPassCreateInfo {
			.name         = "forwardpass",
			.onUpdate     = &ForwardPassUpdate,
			.onRender     = &ForwardPassRender,
			.onBeginFrame = [](const bvk::RenderContext& context) {},

			.colorAttachmentInfos = {
			    bvk::RenderPass::CreateInfo::AttachmentInfo {
			        .name = "forwardpass",
			        .size = { 1.0, 1.0 },
			        .sizeType =
			            bvk::RenderPass::CreateInfo::SizeType::eSwapchainRelative,
			        .format  = colorFormat,
			        .samples = sampleCount,
			        .clearValue =
			            vk::ClearValue {
			                vk::ClearColorValue {
			                    std::array<float, 4>({ 0.3f, 0.5f, 0.8f, 1.0f }) },
			            },
			    },
			},

			.depthStencilAttachmentInfo = bvk::RenderPass::CreateInfo::AttachmentInfo {
			    .name       = "forward_depth",
			    .size       = { 1.0, 1.0 },
			    .sizeType   = bvk::RenderPass::CreateInfo::SizeType::eSwapchainRelative,
			    .format     = deppthFormat,
			    .samples    = sampleCount,
			    .clearValue = vk::ClearValue {
			        vk::ClearDepthStencilValue { 1.0, 0 },
			    },
			},

			.textureInputInfos = {
			    bvk::RenderPass::CreateInfo::TextureInputInfo {
			        .name           = "texture_2ds",
			        .binding        = 0,
			        .count          = 32,
			        .type           = vk::DescriptorType::eCombinedImageSampler,
			        .stageMask      = vk::ShaderStageFlagBits::eFragment,
			        .defaultTexture = defaultTexture,
			    },
			    bvk::RenderPass::CreateInfo::TextureInputInfo {
			        .name           = "texture_cubes",
			        .binding        = 1,
			        .count          = 8u,
			        .type           = vk::DescriptorType::eCombinedImageSampler,
			        .stageMask      = vk::ShaderStageFlagBits::eFragment,
			        .defaultTexture = defaultTextureCube,
			    },
			},
		};

		bvk::RenderPass::CreateInfo uiPassCreateInfo {
			.name         = "ui",
			.onUpdate     = &UserInterfacePassUpdate,
			.onRender     = &UserInterfacePassRender,
			.onBeginFrame = &UserInterfacePassBeginFrame,

			.colorAttachmentInfos = {
			    bvk::RenderPass::CreateInfo::AttachmentInfo {
			        .name = "backbuffer",
			        .size = { 1.0, 1.0 },
			        .sizeType =
			            bvk::RenderPass::CreateInfo::SizeType::eSwapchainRelative,
			        .format  = colorFormat,
			        .samples = sampleCount,
			        .input   = "forwardpass",
			    },
			},
		};

		m_Renderer.BuildRenderGraph({
		    .backbufferName = "backbuffer",
		    .bufferInputs   = {
		          bvk::RenderPass::CreateInfo::BufferInputInfo {
		              .name      = "frame_data",
		              .binding   = 0,
		              .count     = 1,
		              .type      = vk::DescriptorType::eUniformBuffer,
		              .stageMask = vk::ShaderStageFlagBits::eVertex,

		              .size        = (sizeof(glm::mat4) * 2) + (sizeof(glm::vec4)),
		              .initialData = nullptr,
                },
		          bvk::RenderPass::CreateInfo::BufferInputInfo {
		              .name      = "scene_data",
		              .binding   = 1,
		              .count     = 1,
		              .type      = vk::DescriptorType::eUniformBuffer,
		              .stageMask = vk::ShaderStageFlagBits::eVertex,

		              .size        = sizeof(glm::vec4),
		              .initialData = nullptr,
                },
            },
		    .renderPasses = {
		        forwardPassCreateInfo,
		        uiPassCreateInfo,
		    },
		    .onUpdate     = &RenderGraphUpdate,
		    .onBeginFrame = [](const bvk::RenderContext& context) {},
		});
	}
};

Application* CreateApplication()
{
	return new DevelopmentExampleApplication();
}
