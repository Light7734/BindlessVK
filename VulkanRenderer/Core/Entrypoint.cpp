#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Core/Window.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/RenderGraph.hpp"
#include "Graphics/RenderPass.hpp"
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

		materialSystem.LoadShader({
		    name.c_str(),
		    path.c_str(),
		    !strcmp(extension.c_str(), VERTEX_EXTENSION) ? vk::ShaderStageFlagBits::eVertex :
		                                                   vk::ShaderStageFlagBits::eFragment,
		});
	}
}

void LoadShaderEffects(MaterialSystem& materialSystem)
{
	// @TODO: Load from files instead of hard-coding
	materialSystem.CreateShaderEffect({
	    "default",
	    {
	        materialSystem.GetShader("defaultVertex"),
	        materialSystem.GetShader("defaultFragment"),
	    },
	});

	materialSystem.CreateShaderEffect({
	    "skybox",
	    {
	        materialSystem.GetShader("skybox_vertex"),
	        materialSystem.GetShader("skybox_fragment"),
	    },
	});
}

void LoadShaderPasses(MaterialSystem& materialSystem, vk::Format colorAttachmentFormat, vk::Format depthAttachmentFormat, vk::Extent2D extent, vk::SampleCountFlagBits sampleCount)
{
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

		// create shader passes
		materialSystem.CreateShaderPass({
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
		    materialSystem.GetShaderEffect("default"),
		    colorAttachmentFormat,
		    depthAttachmentFormat,
		});
	}

	{
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

		// create shader passes
		materialSystem.CreateShaderPass({
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

		    materialSystem.GetShaderEffect("skybox"),
		    colorAttachmentFormat,
		    depthAttachmentFormat,
		});
	}
}

void LoadMasterMaterials(MaterialSystem& materialSystem)
{
	// @TODO: Load from files instead of hard-coding
	materialSystem.CreateMasterMaterial({
	    "default",
	    materialSystem.GetShaderPass("default"),
	    {},
	});

	materialSystem.CreateMasterMaterial({
	    "skybox",
	    materialSystem.GetShaderPass("skybox"),
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

	materialSystem.CreateMaterial({
	    "skybox",
	    materialSystem.GetMasterMaterial("skybox"),
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

	modelSystem.LoadModel({
	    textureSystem,
	    "skybox",
	    "Assets/Cube/Cube.gltf",
	});
}

void LoadEntities(Scene& scene, MaterialSystem& materialSystem, ModelSystem& modelSystem)
{
	// @TODO: Load from files instead of hard-coding

	Entity testModel = scene.CreateEntity();

	scene.AddComponent<TransformComponent>(testModel,
	                                       glm::vec3(0.0f),          // Translation
	                                       glm::vec3(1.0f),          // Scale
	                                       glm::vec3(0.0f, 0.0, 0.0) // Rotation
	);

	scene.AddComponent<StaticMeshRendererComponent>(testModel,
	                                                materialSystem.GetMaterial("default"), // Material
	                                                modelSystem.GetModel("default")        // Mesh
	);

	Entity skybox = scene.CreateEntity();
	scene.AddComponent<TransformComponent>(skybox,
	                                       glm::vec3(0.0f),          // Translation
	                                       glm::vec3(1.0f),          // Scale
	                                       glm::vec3(0.0f, 0.0, 0.0) // Rotation
	);

	scene.AddComponent<StaticMeshRendererComponent>(skybox,
	                                                materialSystem.GetMaterial("skybox"), // Material
	                                                modelSystem.GetModel("skybox")        // Mesh
	);

	Entity light = scene.CreateEntity();
	LOG(trace, "MOMENT");
	scene.AddComponent<TransformComponent>(light,
	                                       glm::vec3(2.0f, 2.0f, 1.0f), // Translation
	                                       glm::vec3(1.0f),             // Scale
	                                       glm::vec3(0.0f, 0.0, 0.0)    // Rotation
	);

	LOG(trace, "THIS");
	scene.AddComponent<LightComponent>(light, 12);
	LOG(trace, "BRUH");
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
		Device device({
		    .window             = &window,
		    .layers             = { "VK_LAYER_KHRONOS_validation" },
		    .instanceExtensions = {
		        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		    },
		    .logicalDeviceExtensions = {
		        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		    },
		    .enableDebugging      = true,
		    .debugMessageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		    .debugMessageTypes    = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		});
		DeviceContext deviceContext = device.GetContext();


		// textureSystem
		TextureSystem textureSystem({
		    .deviceContext = deviceContext,
		});

		uint8_t defaultTexturePixelData[4] = { 255, 0, 255, 255 };
		textureSystem.CreateFromBuffer({
		    .name   = "default",
		    .pixels = defaultTexturePixelData,
		    .width  = 1,
		    .height = 1,
		    .size   = sizeof(defaultTexturePixelData),
		    .type   = Texture::Type::e2D,
		});


		textureSystem.CreateFromKTX({
		    "skybox",
		    "Assets/cubemap_yokohama_rgba.ktx",
		    Texture::Type::eCubeMap,
		});

		// Renderer
		Renderer renderer({
		    .deviceContext  = deviceContext,
		    .window         = &window,
		    .defaultTexture = textureSystem.GetTexture("default"),
		    .skyboxTexture  = textureSystem.GetTexture("skybox"),
		});

		// MaterialSystem
		MaterialSystem materialSystem({
		    deviceContext.logicalDevice,
		});

		// modelSystem
		ModelSystem modelSystem({
		    deviceContext,
		    renderer.GetCommandPool(),
		    renderer.GetQueueInfo().graphicsQueue,
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
		LoadShaderPasses(materialSystem, deviceContext.surfaceInfo.format.format, deviceContext.depthFormat, deviceContext.surfaceInfo.capabilities.currentExtent, deviceContext.maxSupportedSampleCount);
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
				renderer.RecreateSwapchainResources(&window, device.GetContext());

				materialSystem.DestroyAllMaterials();
				LoadShaderPasses(materialSystem, deviceContext.surfaceInfo.format.format, deviceContext.depthFormat, deviceContext.surfaceInfo.capabilities.currentExtent, deviceContext.maxSupportedSampleCount);
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
