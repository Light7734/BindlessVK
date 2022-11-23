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

		LoadPipelineConfigurations();
		LoadShaderPasses();

		LoadMaterials();

		LoadModels();
		LoadEntities();
		CreateRenderGraph();
	}

	~DevelopmentExampleApplication()
	{
	}

	virtual void OnTick(double deltaTime) override
	{
		m_Renderer.BeginFrame(&m_Scene);

		if (m_Renderer.IsSwapchainInvalidated())
		{
			m_DeviceSystem.UpdateSurfaceInfo();
			m_Renderer.OnSwapchainInvalidated();
			m_CameraController.OnWindowResize(m_Window.GetFramebufferSize().width,
			                                  m_Window.GetFramebufferSize().height);

			LoadMaterials();
		}

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
		    "opaque_mesh",
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

	void LoadPipelineConfigurations()
	{
		bvk::Device* device = m_DeviceSystem.GetDevice();

		m_MaterialSystem.CreatePipelineConfiguration({
		    .name               = "opaque_mesh",
		    .vertexInputState   = bvk::VertexTypes::Model::GetVertexInputState(),
		    .inputAssemblyState = {
		        {}, // flags
		        vk::PrimitiveTopology::eTriangleList,
		        VK_FALSE,
		    },
		    .tessellationState = {},
		    .viewportState     = {
		            {}, // flags
		            1u, // viewportCount
		            {}, // pViewports
		            1u, // scissorCount
		            {}, // pScissors
            },
		    .rasterizationState = {
		        {},                          // flags
		        VK_FALSE,                    // depthClampEnable
		        VK_FALSE,                    // rasterizerdiscardEnable
		        vk::PolygonMode::eFill,      // polygonMode
		        vk::CullModeFlagBits::eBack, // cullMode
		        vk::FrontFace::eClockwise,   // frontFace
		        VK_FALSE,                    // depthBiasEnable
		        0.0f,                        // depthBiasConstantFactor
		        0.0f,                        // depthBiasClamp
		        0.0f,                        // depthBiasSlopeFactor
		        1.0f,                        // lineWidth
		    },
		    .multisampleState = {
		        {},
		        device->maxDepthColorSamples,
		        VK_FALSE,
		        {},
		        VK_FALSE,
		        VK_FALSE,
		    },
		    .depthStencilState = {
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
		    .colorBlendAttachments = {
		        {
		            VK_FALSE,                           // blendEnable
		            vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
		            vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
		            vk::BlendOp::eAdd,                  // colorBlendOp
		            vk::BlendFactor::eOne,              // srcAlphaBlendFactor
		            vk::BlendFactor::eZero,             // dstAlphaBlendFactor
		            vk::BlendOp::eAdd,                  // alphaBlendOp

		            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA // colorWriteMask
		        },
		    },
		    .colorBlendState = {},
		    .dynamicStates   = {
		          vk::DynamicState::eViewport,
		          vk::DynamicState::eScissor,
            },
		});

		m_MaterialSystem.CreatePipelineConfiguration({
		    .name               = "skybox",
		    .vertexInputState   = bvk::VertexTypes::Model::GetVertexInputState(),
		    .inputAssemblyState = {
		        {},
		        vk::PrimitiveTopology::eTriangleList,
		        VK_FALSE,
		    },
		    .tessellationState = {},
		    .viewportState     = {
		            {}, // flags
		            1u, // viewportCount
		            {}, // pViewports
		            1u, // scissorCount
		            {}, // pScissors
            },
		    .rasterizationState = {
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
		    .multisampleState = {
		        {},
		        device->maxDepthColorSamples,
		        VK_FALSE,
		        {},
		        VK_FALSE,
		        VK_FALSE,
		    },
		    .depthStencilState = {
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
		    .colorBlendAttachments = {
		        {
		            VK_FALSE,                           // blendEnable
		            vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
		            vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
		            vk::BlendOp::eAdd,                  // colorBlendOp
		            vk::BlendFactor::eOne,              // srcAlphaBlendFactor
		            vk::BlendFactor::eZero,             // dstAlphaBlendFactor
		            vk::BlendOp::eAdd,                  // alphaBlendOp

		            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA // colorWriteMask
		        },
		    },
		    .colorBlendState = {},
		    .dynamicStates   = {
		          vk::DynamicState::eViewport,
		          vk::DynamicState::eScissor,
            },
		});
	}

	void LoadShaderPasses()
	{
		bvk::Device* device = m_DeviceSystem.GetDevice();

		// create shader passes
		m_MaterialSystem.CreateShaderPass({
		    .name   = "opaque_mesh",
		    .effect = m_MaterialSystem.GetShaderEffect("opaque_mesh"),

		    .colorAttachmentFormat = device->surfaceFormat.format,
		    .depthAttachmentFormat = device->depthFormat,

		    .pipelineConfiguration = m_MaterialSystem.GetPipelineConfiguration("opaque_mesh"),
		});

		m_MaterialSystem.CreateShaderPass({
		    .name   = "skybox",
		    .effect = m_MaterialSystem.GetShaderEffect("skybox"),

		    .colorAttachmentFormat = device->surfaceFormat.format,
		    .depthAttachmentFormat = device->depthFormat,

		    .pipelineConfiguration = m_MaterialSystem.GetPipelineConfiguration("skybox"),
		});
	}

	void LoadMaterials()
	{
		// @TODO: Load from files instead of hard-coding
		m_MaterialSystem.CreateMaterial({
		    .name       = "opaque_mesh",
		    .shaderPass = m_MaterialSystem.GetShaderPass("opaque_mesh"),
		    .parameters = {},
		    .textures   = {},
		});

		m_MaterialSystem.CreateMaterial({
		    .name       = "skybox",
		    .shaderPass = m_MaterialSystem.GetShaderPass("skybox"),
		    .parameters = {},
		    .textures   = {},
		});
	}

	void LoadModels()
	{
		// @TODO: Load from files instead of hard-coding
		m_ModelSystem.LoadModel({
		    m_TextureSystem,
		    "flight_helmet",
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
		    m_MaterialSystem.GetMaterial("opaque_mesh"), // Material
		    m_ModelSystem.GetModel("flight_helmet")      // Mesh
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
