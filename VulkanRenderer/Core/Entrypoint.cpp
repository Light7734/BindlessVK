#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Core/Window.hpp"
#include "Graphics/Device.hpp"
#include "Scene/Camera.hpp"
// #include "Graphics/Pipeline.hpp"
#include "Graphics/Renderer.hpp"
// #include "Graphics/Shader.hpp"
#include "Scene/Scene.hpp"
#include "Utils/Timer.hpp"

#include <filesystem>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

void LoadShaders(MaterialSystem& materialSystem)
{
	const char* const DIRECTORY          = "Shaders/";
	const char* const VERTEX_EXTENSION   = ".spv_vs";
	const char* const FRAGMENT_EXTENSION = ".spv_fs";

	for (auto& shader : std::filesystem::directory_iterator(DIRECTORY))
	{
		std::string path(shader.path().c_str());
		std::string extension(shader.path().extension().c_str());
		std::string name(shader.path().filename().replace_extension().c_str());

		if (strcmp(extension.c_str(), VERTEX_EXTENSION) && strcmp(extension.c_str(), FRAGMENT_EXTENSION))
		{
			continue;
		}

		Shader::CreateInfo shaderCreateInfo {
			name.c_str(),
			path.c_str(),
			!strcmp(extension.c_str(), VERTEX_EXTENSION) ? vk::ShaderStageFlagBits::eVertex :
			                                               vk::ShaderStageFlagBits::eFragment,
		};
		LOG(trace, "Loaded shader: {} -> {}", name, path);

		materialSystem.LoadShader(shaderCreateInfo);
	}
}

void LoadShaderEffects(MaterialSystem& materialSystem)
{
	// @TODO: Load from files instead of hard-coding
	ShaderEffect::CreateInfo shaderEffectCreateInfo {
		"default",
		{
		    materialSystem.GetShader("defaultVertex"),
		    materialSystem.GetShader("defaultFragment"),
		},
	};

	materialSystem.CreateShaderEffect(shaderEffectCreateInfo);
}

void LoadShaderPasses(MaterialSystem& materialSystem, vk::RenderPass forwardPass, vk::Extent2D extent, vk::SampleCountFlagBits sampleCount)
{
	// @TODO: Load from files instead of hard-coding
	std::vector<vk::VertexInputBindingDescription> inputBindings {
		vk::VertexInputBindingDescription {
		    0u,                                           // binding
		    static_cast<uint32_t>(sizeof(Model::Vertex)), // stride
		    vk::VertexInputRate::eVertex,                 // inputRate
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
		    4u,                                                                            // location
		    0u,                                                                            // binding
		    vk::Format::eR32G32B32Sfloat,                                                  // format
		    sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3), // offset
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

		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA, // colorWriteMask
	};

	PipelineConfiguration defaultPassPipelineConfigurations {
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
	};

	// create shader passes
	materialSystem.CreateShaderPass({
	    "default",
	    defaultPassPipelineConfigurations,
	    materialSystem.GetShaderEffect("default"),
	    forwardPass,
	    0ull,
	});
}

void LoadMasterMaterials(MaterialSystem& materialSystem)
{
	// @TODO: Load from files instead of hard-coding
	materialSystem.CreateMasterMaterial({
	    "default",
	    materialSystem.GetShaderPass("default"),
	    {},
	});
}

void LoadMaterials(MaterialSystem& materialSystem)
{
	// @TODO: Load from files instead of hard-coding
	materialSystem.CreateMaterial({
	    "default",
	    materialSystem.GetMasterMaterial("default"),
	    {},
	    {},
	});
}

void LoadModels(ModelSystem& modelSystem, TextureSystem& textureSystem)
{
	// @TODO: Load from files instead of hard-coding
	modelSystem.LoadModel({
	    textureSystem,
	    "default",
	    "Assets/FlightHelmet/FlightHelmet.gltf",
	});
}

void LoadEntities(Scene& scene, MaterialSystem& materialSystem, ModelSystem& modelSystem)
{
	// @TODO: Load from files instead of hard-coding

	Entity entity = scene.CreateEntity();

	scene.AddComponent<TransformComponent>(entity,
	                                       glm::vec3(0.0f),          // Translation
	                                       glm::vec3(1.0f),          // Scale
	                                       glm::vec3(0.0f, 0.0, 0.0) // Rotation
	);

	scene.AddComponent<StaticMeshRendererComponent>(entity,
	                                                materialSystem.GetMaterial("default"), // Material
	                                                modelSystem.GetModel("default")        // Mesh
	);
}

int main()
{
	Logger::Init();

	int exitCode = 0;
	try
	{
		/////////////////////////////////////////////////////////////////////////////////
		/// Initialize application
		// Window
		Window window({
		    .specs = {
		        .title  = "BindlessVk",
		        .width  = 1920u,
		        .height = 1080u,
		    },
		    .hints = {
		        { GLFW_CLIENT_API, GLFW_NO_API },
		        { GLFW_FLOATING, GLFW_TRUE },
		    },
		});

		// Device
		Device device(DeviceCreateInfo {
		    .window             = &window,
		    .layers             = { "VK_LAYER_KHRONOS_validation" },
		    .instanceExtensions = {
		        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		    },
		    .logicalDeviceExtensions = {
		        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		    },
		    .enableDebugging      = true,
		    .debugMessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
		    .debugMessageTypes    = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		});

		// Renderer
		Renderer renderer = Renderer({
		    .window        = &window,
		    .deviceContext = device.GetContext(),
		});

		// MaterialSystem
		MaterialSystem materialSystem({
		    device.GetContext().logicalDevice,
		});

		// modelSystem
		ModelSystem modelSystem({
		    device.GetContext(),
		    renderer.GetCommandPool(),
		    renderer.GetQueueInfo().graphicsQueue,
		});

		// textureSystem
		TextureSystem textureSystem({
		    .deviceContext = device.GetContext(),
		});

		// Scene
		Scene scene;
		Camera camera({
		    .position  = { 2.0f, 2.0f, 0.5f },
		    .speed     = 1.0f,
		    .nearPlane = 0.001f,
		    .farPlane  = 100.0f,
		    .width     = 5.0f,
		    .height    = 5.0f,
		    .window    = &window,
		});

		/////////////////////////////////////////////////////////////////////////////////
		/// Load assets
		LoadShaders(materialSystem);
		LoadShaderEffects(materialSystem);
		LoadShaderPasses(materialSystem, renderer.GetForwardPass(), device.GetContext().surfaceInfo.capabilities.currentExtent, device.GetContext().maxSupportedSampleCount);
		LoadMasterMaterials(materialSystem);
		LoadMaterials(materialSystem);

		LoadModels(modelSystem, textureSystem);

		LoadEntities(scene, materialSystem, modelSystem);

		/////////////////////////////////////////////////////////////////////////////////
		/// Main application loop
		uint32_t frames = 0u;
		Timer fpsTimer;

		while (!window.ShouldClose())
		{
			window.PollEvents();

			camera.Update();

			renderer.BeginFrame();
			renderer.DrawScene(&scene, camera);

			if (renderer.IsSwapchainInvalidated())
			{
				renderer.RecreateSwapchain(&window, device.GetContext());

				materialSystem.DestroyAllMaterials();
				LoadShaderPasses(materialSystem, renderer.GetForwardPass(), device.GetContext().surfaceInfo.capabilities.currentExtent, device.GetContext().maxSupportedSampleCount);
				LoadMasterMaterials(materialSystem);
				LoadMaterials(materialSystem);

				scene.Reset();
				LoadEntities(scene, materialSystem, modelSystem);
			}

			// FPS Counter
			frames++;
			if (fpsTimer.ElapsedTime() >= 1.0f)
			{
				LOG(trace, "FPS: {}", frames);
				frames = 0;
				fpsTimer.Reset();
			}
		}
	}
	catch (FailedAsssertionException exception)
	{
		LOG(critical, "Terminating due FailedAssertionException");
		exitCode = 1;
	}

	return exitCode;
}
