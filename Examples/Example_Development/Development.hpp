#pragma once

#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Renderer.hpp"
#include "BindlessVk/VkContext.hpp"
#include "Framework/Common/Common.hpp"
//
#include "Framework/Core/Application.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/Timer.hpp"

// passes
#include "ForwardPass.hpp"
#include "Graph.hpp"
#include "UserInterfacePass.hpp"

// @todo: load stuff from files
class DevelopmentExampleApplication: public Application
{
public:
	DevelopmentExampleApplication(): Application()
	{
		load_shaders();
		load_pipeline_configuration();
		load_shader_effects();
		load_materials();

		load_models();

		load_entities();

		create_render_graph();
	}

	~DevelopmentExampleApplication()
	{
	}

	virtual void on_tick(double deltaTime) override
	{
		renderer.begin_frame(&scene);

		if (renderer.is_swapchain_invalidated()) {
			vk_context.update_surface_info();
			renderer.on_swapchain_invalidated();
			camera_controller.on_window_resize(
			  window.get_framebuffer_size().width,
			  window.get_framebuffer_size().height
			);

			logger.log(spdlog::level::trace, "re-loading materials");
			load_materials();
		}

		ImGui::ShowDemoWindow();
		CVar::draw_imgui_editor();

		camera_controller.update();
		renderer.end_frame(&scene);
	}

	virtual void on_swapchain_recreate() override
	{
		assert_fail("Swapchain recreation not supported (yet)");
	}

private:
	void load_shaders()
	{
		c_str DIRECTORY = "Shaders/";

		for (auto& shader_file : std::filesystem::directory_iterator(DIRECTORY)) {
			const str path(shader_file.path().c_str());
			const str name(shader_file.path().filename().replace_extension().c_str());
			const str extension(shader_file.path().extension().c_str());

			if (strcmp(extension.c_str(), ".spv")) {
				continue;
			}

			shaders[hash_str(name.c_str())] = shader_loader.load_from_spv(path.c_str());
			logger.log(spdlog::level::trace, "Loaded shader {}", name);
		}
	}

	void load_shader_effects()
	{
		shader_effects[hash_str("opaque_mesh")] = bvk::ShaderEffect(
		 &vk_context,
		  {
		    &shaders[hash_str("vertex")],
		    &shaders[hash_str("pixel")],
		  },
		  shader_effect_configurations[hash_str("opaque_mesh")]
		);

		shader_effects[hash_str("skybox")] = bvk::ShaderEffect(
		  &vk_context,
		  {
		    &shaders[hash_str("skybox_vertex")],
		    &shaders[hash_str("skybox_fragment")],
		  },
		  shader_effect_configurations[hash_str("skybox")]
		);
	}

	void load_pipeline_configuration()
	{
		shader_effect_configurations[hash_str("opaque_mesh")] = bvk::ShaderEffect::Configuration {
			bvk::Model::Vertex::get_vertex_input_state(),
			vk::PipelineInputAssemblyStateCreateInfo {
			  {},
			  vk::PrimitiveTopology::eTriangleList,
			  VK_FALSE,
			},
			vk::PipelineTessellationStateCreateInfo {},
			vk::PipelineViewportStateCreateInfo {
			  {},
			  1u,
			  {},
			  1u,
			  {},
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
			  vk_context.get_max_color_and_depth_samples(),
			  VK_FALSE,
			  {},
			  VK_FALSE,
			  VK_FALSE,
			},
			vk::PipelineDepthStencilStateCreateInfo {
			  {},
			  VK_TRUE,
			  VK_TRUE,
			  vk::CompareOp::eLess,
			  VK_FALSE,
			  VK_FALSE,
			  {},
			  {},
			  0.0f,
			  1.0,
			},
			vk::PipelineColorBlendStateCreateInfo {},
			std::vector<vk::PipelineColorBlendAttachmentState> {
			  {
			    VK_FALSE,
			    vk::BlendFactor::eSrcAlpha,
			    vk::BlendFactor::eOneMinusSrcAlpha,
			    vk::BlendOp::eAdd,
			    vk::BlendFactor::eOne,
			    vk::BlendFactor::eZero,
			    vk::BlendOp::eAdd,
			    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
			      | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
			  },
			},
			std::vector<vk::DynamicState> {
			  vk::DynamicState::eViewport,
			  vk::DynamicState::eScissor,
			},
		};

		shader_effect_configurations[hash_str("skybox")] = bvk::ShaderEffect::Configuration {
			bvk::Model::Vertex::get_vertex_input_state(),
			vk::PipelineInputAssemblyStateCreateInfo {
			  {},
			  vk::PrimitiveTopology::eTriangleList,
			  VK_FALSE,
			},
			vk::PipelineTessellationStateCreateInfo {},
			vk::PipelineViewportStateCreateInfo {
			  {},
			  1u,
			  {},
			  1u,
			  {},
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
			  vk_context.get_max_color_and_depth_samples(),
			  VK_FALSE,
			  {},
			  VK_FALSE,
			  VK_FALSE,
			},
			vk::PipelineDepthStencilStateCreateInfo {
			  {},
			  VK_TRUE,
			  VK_TRUE,
			  vk::CompareOp::eLessOrEqual,
			  VK_FALSE,
			  VK_FALSE,
			  {},
			  {},
			  0.0f,
			  1.0,
			},
			vk::PipelineColorBlendStateCreateInfo {},
			std::vector<vk::PipelineColorBlendAttachmentState> {
			  {
			    VK_FALSE,
			    vk::BlendFactor::eSrcAlpha,
			    vk::BlendFactor::eOneMinusSrcAlpha,
			    vk::BlendOp::eAdd,
			    vk::BlendFactor::eOne,
			    vk::BlendFactor::eZero,
			    vk::BlendOp::eAdd,
			    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
			      | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
			  },
			},
			std::vector<vk::DynamicState> {
			  vk::DynamicState::eViewport,
			  vk::DynamicState::eScissor,
			},
		};
	}

	void load_materials()
	{
		materials.emplace(
		  hash_str("opaque_mesh"),
		  bvk::Material(&vk_context, &shader_effects[hash_str("opaque_mesh")], descriptor_pool)
		);

		materials.emplace(
		  hash_str("skybox"),
		  bvk::Material(&vk_context, &shader_effects[hash_str("skybox")], descriptor_pool)
		);
	}

	void load_models()
	{
		models[bvk::hash_str("flight_helmet")] = model_loader.load_from_gltf_ascii(
		  "flight_helmet",
		  "Assets/FlightHelmet/FlightHelmet.gltf",
		  staging_pool.get_by_index(0u),
		  staging_pool.get_by_index(1u),
		  staging_pool.get_by_index(2u)
		);

		models[bvk::hash_str("skybox")] = model_loader.load_from_gltf_ascii(
		  "skybox",
		  "Assets/Cube/Cube.gltf",
		  staging_pool.get_by_index(0u),
		  staging_pool.get_by_index(1u),
		  staging_pool.get_by_index(2u)
		);
	}

	// @todo: Load from files instead of hard-coding
	void load_entities()
	{
		Entity testModel = scene.create();

		scene.emplace<TransformComponent>(
		  testModel,
		  glm::vec3(0.0f),
		  glm::vec3(1.0f),
		  glm::vec3(0.0f, 0.0, 0.0)
		);

		scene.emplace<StaticMeshRendererComponent>(
		  testModel,
		  &materials.at(hash_str("opaque_mesh")),
		  &models[bvk::hash_str("flight_helmet")]
		);

		Entity skybox = scene.create();
		scene.emplace<TransformComponent>(
		  skybox,
		  glm::vec3(0.0f),
		  glm::vec3(1.0f),
		  glm::vec3(0.0f, 0.0, 0.0)
		);

		scene.emplace<StaticMeshRendererComponent>(
		  skybox,
		  &materials[hash_str("skybox")],
		  &models[bvk::hash_str("skybox")]
		);

		Entity light = scene.create();
		scene.emplace<TransformComponent>(
		  light,
		  glm::vec3(2.0f, 2.0f, 1.0f),
		  glm::vec3(1.0f),
		  glm::vec3(0.0f, 0.0, 0.0)
		);

		Entity camera = scene.create();
		scene.emplace<TransformComponent>(
		  camera,
		  glm::vec3(6.0, 7.0, 2.5),
		  glm::vec3(1.0),
		  glm::vec3(0.0f, 0.0, 0.0)
		);

		scene.emplace<CameraComponent>(
		  camera,
		  45.0f,
		  5.0,
		  1.0,
		  0.001f,
		  100.0f,
		  225.0,
		  0.0,
		  glm::vec3(0.0f, 0.0f, -1.0f),
		  glm::vec3(0.0f, -1.0f, 0.0f),
		  10.0f
		);

		scene.emplace<LightComponent>(light, 12);
	}

	void create_render_graph()
	{
		const auto& surface = vk_context.get_surface();

		const auto color_format = surface.color_format;
		const auto depth_format = vk_context.get_depth_format();
		const auto sample_count = vk_context.get_max_color_and_depth_samples();

		auto* default_texture = &textures[hash_str("default_2d")];
		auto* default_texture_cube = &textures[hash_str("default_cube")];

		const std::array<float, 4> update_color = { 1.0, 0.8, 0.8, 1.0 };
		const std::array<float, 4> barrier_color = { 0.8, 1.0, 0.8, 1.0 };
		const std::array<float, 4> render_color = { 0.8, 0.8, 1.0, 1.0 };

		bvk::Renderpass::CreateInfo forward_pass_info {
                 "forwardpass",
                     {},
                 &forward_pass_update,
                 &forward_pass_render,
                 {
                    bvk::Renderpass::CreateInfo::AttachmentInfo {
                        .name = "forwardpass",
                        .size = { 1.0, 1.0 },
                        .size_type =
                            bvk::Renderpass::CreateInfo::SizeType::eSwapchainRelative,
                        .format  = color_format,
                        .samples = sample_count,
                        .clear_value =
                            vk::ClearValue{
                                vk::ClearColorValue {
                                    std::array<float, 4>({ 0.3f, 0.5f, 0.8f, 1.0f }) },
                            },
                    },
                },

                bvk::Renderpass::CreateInfo::AttachmentInfo {
                    .name       = "forward_depth",
                    .size       = { 1.0, 1.0 },
                    .size_type   = bvk::Renderpass::CreateInfo::SizeType::eSwapchainRelative,
                    .format     = depth_format,
                    .samples    = sample_count,
                    .clear_value = vk::ClearValue {
                        vk::ClearDepthStencilValue { 1.0, 0 },
                    },
                },

                 {
                    bvk::Renderpass::CreateInfo::TextureInputInfo {
                        .name           = "texture_2ds",
                        .binding        = 0,
                        .count          = 32,
                        .type           = vk::DescriptorType::eCombinedImageSampler,
                        .stage_mask      = vk::ShaderStageFlagBits::eFragment,
                        .default_texture = default_texture,
                    },
                    bvk::Renderpass::CreateInfo::TextureInputInfo {
                        .name           = "texture_cubes",
                        .binding        = 1,
                        .count           = 8u,
                        .type  = vk::DescriptorType::eCombinedImageSampler,
                        .stage_mask = vk::ShaderStageFlagBits::eFragment,
                        .default_texture=default_texture_cube,
                    },
                },
                {},
                {},
               vk::DebugUtilsLabelEXT({
                        "forwardpass_update",
                        update_color,
                }),

               vk::DebugUtilsLabelEXT({
                        "forwardpass_barrier",
                        barrier_color,
                }),
               vk::DebugUtilsLabelEXT({
                        "forwardpass_render",
                        render_color,
                }),
            };

		bvk::Renderpass::CreateInfo ui_pass_info {
			"uipass",
			&user_interface_pass_begin_frame,
			&user_interface_pass_update,
			&user_interface_pass_render,
			{
			  bvk::Renderpass::CreateInfo::AttachmentInfo {
			    .name = "backbuffer",
			    .size = { 1.0, 1.0 },
			    .size_type = bvk::Renderpass::CreateInfo::SizeType::eSwapchainRelative,
			    .format = color_format,
			    .samples = sample_count,
			    .input = "forwardpass",
			  },
			},
			{},
			{},
			{},
			{},
			vk::DebugUtilsLabelEXT({
			  "uipass_update",
			  update_color,
			}),

			vk::DebugUtilsLabelEXT({
			  "uipass_barrier",
			  barrier_color,
			}),
			vk::DebugUtilsLabelEXT({
			  "uipass_render",
			  render_color,
			}),
		};

		renderer.build_render_graph(
		  "backbuffer",
		  {
		    bvk::Renderpass::CreateInfo::BufferInputInfo {
		      .name = "frame_data",
		      .binding = 0,
		      .count = 1,
		      .type = vk::DescriptorType::eUniformBuffer,
		      .stage_mask = vk::ShaderStageFlagBits::eVertex,

		      .size = (sizeof(glm::mat4) * 2) + (sizeof(glm::vec4)),
		      .initial_data = nullptr,
		    },
		    bvk::Renderpass::CreateInfo::BufferInputInfo {
		      .name = "scene_data",
		      .binding = 1,
		      .count = 1,
		      .type = vk::DescriptorType::eUniformBuffer,
		      .stage_mask = vk::ShaderStageFlagBits::eVertex,

		      .size = sizeof(glm::vec4),
		      .initial_data = nullptr,
		    },
		  },
		  {
		    forward_pass_info,
		    ui_pass_info,
		  },
		  &render_graph_update,
		  nullptr,
		  vk::DebugUtilsLabelEXT({
		    "graph_update",
		    update_color,
		  }),
		  vk::DebugUtilsLabelEXT({
		    "backbuffer_present_barrier",
		    barrier_color,
		  })
		);
	}
};
