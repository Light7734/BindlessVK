#include "Development/Development.hpp"

#include <span>

DevelopmentExampleApplication::DevelopmentExampleApplication()
{
	load_shaders();
	load_pipeline_configuration();
	load_shader_effects();
	load_materials();

	load_models();

	load_entities();

	create_render_graph();
}

DevelopmentExampleApplication::~DevelopmentExampleApplication()
{
}

void DevelopmentExampleApplication::on_tick(f64 const delta_time)
{
	ImGui::ShowDemoWindow();
	CVar::draw_imgui_editor();

	camera_controller.update();
	renderer->render_graph(render_graph.get());

	if (vk_context->get_swapchain()->is_invalid())
		assert_fail("swapchain-recreation is currently nuked");
}

void DevelopmentExampleApplication::load_shaders()
{
	auto constexpr DIRECTORY = "Shaders/";

	for (auto const &shader_file : std::filesystem::directory_iterator(DIRECTORY))
	{
		str const path(shader_file.path().c_str());
		str const extension(shader_file.path().extension().c_str());
		str const name(shader_file.path().filename().replace_extension());

		if (strcmp(extension.c_str(), ".spv"))
			continue;

		shaders[hash_str(name)] = shader_loader.load_from_spv(path);
		logger.log(spdlog::level::trace, "Loaded shader {}", name);
	}
}

void DevelopmentExampleApplication::load_shader_effects()
{
	auto const device = vk_context->get_device();

	auto const descriptor_set_bindings = BasicRendergraph::get_descriptor_set_bindings();
	auto const layout_allocator = vk_context->get_layout_allocator();

	auto const [bindings, flags] = BasicRendergraph::get_descriptor_set_bindings();
	auto const graph_set_layout = layout_allocator->goc_descriptor_set_layout({}, bindings, flags);

	shader_pipelines.emplace(
	    hash_str("opaque_mesh"),

	    bvk::ShaderPipeline(
	        vk_context.get(),
	        {
	            &shaders[hash_str("vertex")],
	            &shaders[hash_str("pixel")],
	        },
	        shader_effect_configurations[hash_str("opaque_mesh")],
	        graph_set_layout,
	        {},
	        "opaque_mesh"
	    )
	);

	shader_pipelines.emplace(
	    hash_str("skybox"),

	    bvk::ShaderPipeline(
	        vk_context.get(),
	        {
	            &shaders[hash_str("skybox_vertex")],
	            &shaders[hash_str("skybox_fragment")],
	        },
	        shader_effect_configurations[hash_str("skybox")],
	        graph_set_layout,
	        {},
	        "skybox"
	    )
	);
}

void DevelopmentExampleApplication::load_pipeline_configuration()
{
	shader_effect_configurations[hash_str("opaque_mesh")] = bvk::ShaderPipeline::Configuration {
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
		    vk_context->get_gpu().get_max_color_and_depth_samples(),
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
		vec<vk::PipelineColorBlendAttachmentState> {
		    vk::PipelineColorBlendAttachmentState {
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
		vec<vk::DynamicState> {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor,
		},
	};

	shader_effect_configurations[hash_str("skybox")] = bvk::ShaderPipeline::Configuration {
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
		    vk_context->get_gpu().get_max_color_and_depth_samples(),
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
		vec<vk::PipelineColorBlendAttachmentState> {
		    vk::PipelineColorBlendAttachmentState {
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
		vec<vk::DynamicState> {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor,
		},
	};
}

void DevelopmentExampleApplication::load_materials()
{
	materials.emplace(
	    hash_str("opaque_mesh"),
	    bvk::Material(
	        vk_context.get(),
	        &shader_pipelines.at(hash_str("opaque_mesh")),
	        descriptor_pool
	    )
	);

	materials.emplace(
	    hash_str("skybox"),
	    bvk::Material(vk_context.get(), &shader_pipelines.at(hash_str("skybox")), descriptor_pool)
	);
}

void DevelopmentExampleApplication::load_models()
{
	models.emplace(
	    hash_str("flight_helmet"),
	    model_loader.load_from_gltf_ascii(
	        "Assets/FlightHelmet/FlightHelmet.gltf",
	        staging_pool.get_by_index(0u),
	        staging_pool.get_by_index(1u),
	        staging_pool.get_by_index(2u),
	        "flight_helmet"
	    )
	);

	models.emplace(
	    hash_str("skybox"),
	    model_loader.load_from_gltf_ascii(
	        "Assets/Cube/Cube.gltf",
	        staging_pool.get_by_index(0u),
	        staging_pool.get_by_index(1u),
	        staging_pool.get_by_index(2u),
	        "skybox"
	    )
	);
}

// @todo: Load from files instead of hard-coding
void DevelopmentExampleApplication::load_entities()
{
	auto const test_model_entity = scene.create();

	scene.emplace<TransformComponent>(
	    test_model_entity,
	    glm::vec3(0.0f),
	    glm::vec3(1.0f),
	    glm::vec3(0.0f, 0.0, 0.0)
	);

	scene.emplace<StaticMeshRendererComponent>(
	    test_model_entity,
	    &materials.at(hash_str("opaque_mesh")),
	    &models.at(hash_str("flight_helmet"))
	);

	auto const skybox_entity = scene.create();
	scene.emplace<TransformComponent>(
	    skybox_entity,
	    glm::vec3(0.0f),
	    glm::vec3(1.0f),
	    glm::vec3(0.0f, 0.0, 0.0)
	);

	scene.emplace<SkyboxComponent>(
	    skybox_entity,
	    &materials.at(hash_str("skybox")),
	    &textures.at(hash_str("default_texture_cube")),
	    &models.at(hash_str("skybox"))
	);

	auto const light_entity = scene.create();
	scene.emplace<TransformComponent>(
	    light_entity,
	    glm::vec3(2.0f, 2.0f, 1.0f),
	    glm::vec3(1.0f),
	    glm::vec3(0.0f, 0.0, 0.0)
	);

	auto const camera_entity = scene.create();
	scene.emplace<TransformComponent>(
	    camera_entity,
	    glm::vec3(6.0, 7.0, 2.5),
	    glm::vec3(1.0),
	    glm::vec3(0.0f, 0.0, 0.0)
	);

	scene.emplace<CameraComponent>(
	    camera_entity,
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

	scene.emplace<LightComponent>(light_entity, 12);
}

auto DevelopmentExampleApplication::create_forward_pass_blueprint() -> bvk::RenderpassBlueprint
{
	auto const &surface = vk_context->get_surface();

	auto const color_format = surface.get_color_format();
	auto const depth_format = vk_context->get_depth_format();
	auto const sample_count = vk_context->get_gpu().get_max_color_and_depth_samples();

	auto const color_output_hash = hash_str("forward_color_out");
	auto const depth_attachment_hash = hash_str("forward_depth");

	auto blueprint = bvk::RenderpassBlueprint {};
	blueprint.set_name("forwardpass")
	    .set_user_data(&scene)
	    .set_sample_count(sample_count)
	    .set_update_label(Forwardpass::get_update_label())
	    .set_render_label(Forwardpass::get_render_label())
	    .set_barrier_label(Forwardpass::get_barrier_label())
	    .add_color_output({
	        color_output_hash,
	        {},
	        {},
	        { 1.0, 1.0 },
	        bvk::Renderpass::SizeType::eSwapchainRelative,
	        color_format,
	        vk::ClearColorValue { 0.3f, 0.5f, 0.8f, 1.0f },
	        "forwardpass_depth",
	    })
	    .set_depth_attachment({
	        depth_attachment_hash,
	        {},
	        {},
	        { 1.0, 1.0 },
	        bvk::Renderpass::SizeType::eSwapchainRelative,
	        depth_format,
	        vk::ClearDepthStencilValue { 1.0, 0 },
	        "forwardpass_depth",
	    });

	return blueprint;
}

auto DevelopmentExampleApplication::create_ui_pass_blueprint() -> bvk::RenderpassBlueprint
{
	auto const &surface = vk_context->get_surface();

	auto const color_format = surface.get_color_format();
	auto const sample_count = vk_context->get_gpu().get_max_color_and_depth_samples();

	auto const color_output_hash = hash_str("uipass_color_out");
	auto const color_output_input_hash = hash_str("forward_color_out");

	auto blueprint = bvk::RenderpassBlueprint {};
	blueprint.set_name("uipass")
	    .set_user_data(&scene)
	    .set_sample_count(sample_count)
	    .set_update_label(UserInterfacePass::get_update_label())
	    .set_render_label(UserInterfacePass::get_render_label())
	    .set_barrier_label(UserInterfacePass::get_barrier_label())
	    .add_color_output({
	        color_output_hash,
	        color_output_input_hash,
	        {},
	        { 1.0, 1.0 },
	        bvk::Renderpass::SizeType::eSwapchainRelative,
	        color_format,
	        {},
	        "uipass_color",
	    });

	return blueprint;
}

void DevelopmentExampleApplication::create_render_graph()
{
	auto const forwrard_pass_blueprint = create_forward_pass_blueprint();
	auto const ui_pass_blueprint = create_ui_pass_blueprint();

	auto builder = bvk::RenderGraphBuilder { vk_context };

	builder.set_type<BasicRendergraph>()
	    .set_resources(renderer->get_resources())
	    .set_user_data(&scene)
	    .set_update_label(BasicRendergraph::get_update_label())
	    .set_present_barrier_label(BasicRendergraph::get_barrier_label())
	    .add_buffer_input({
	        "frame_data",
	        0,
	        1,
	        vk::DescriptorType::eUniformBuffer,
	        vk::ShaderStageFlagBits::eVertex,
	        sizeof(BasicRendergraph::FrameData),
	    })
	    .add_buffer_input({
	        "scene_data",
	        1,
	        1,
	        vk::DescriptorType::eUniformBuffer,
	        vk::ShaderStageFlagBits::eVertex,
	        sizeof(BasicRendergraph::SceneData),
	    })
	    .add_texture_input({
	        "textures",
	        2,
	        10'000,
	        vk::DescriptorType::eCombinedImageSampler,
	        vk::ShaderStageFlagBits::eFragment,
	    })
	    .add_texture_input({
	        "texture_cubes",
	        3,
	        1'000,
	        vk::DescriptorType::eCombinedImageSampler,
	        vk::ShaderStageFlagBits::eFragment,
	    })
	    .add_pass<Forwardpass>(forwrard_pass_blueprint)
	    .add_pass<UserInterfacePass>(ui_pass_blueprint);

	render_graph = scope<bvk::Rendergraph>(builder.build_graph());
}
