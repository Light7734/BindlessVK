#include "Development/Development.hpp"

#include <optional>
#include <span>

DevelopmentExampleApplication::DevelopmentExampleApplication()
{
	load_shaders();
	load_pipeline_configuration();
	load_graphics_pipelines();
	load_compute_pipelines();
	load_materials();

	load_models();

	load_entities();

	initialize_render_nodes();
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
	renderer.render_graph(&render_graph);

	if (renderer.is_swapchain_invalid())
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

void DevelopmentExampleApplication::load_graphics_pipelines()
{
	auto const [bindings, flags] = BasicRendergraph::get_graphics_descriptor_set_bindings();
	auto const graph_set_layout = layout_allocator.goc_descriptor_set_layout({}, bindings, flags);

	shader_pipelines.emplace(
	    hash_str("opaque_mesh"),

	    bvk::ShaderPipeline {
	        &vk_context,
	        &layout_allocator,
	        bvk::ShaderPipeline::Type::eGraphics,
	        {
	            &shaders[hash_str("vertex")],
	            &shaders[hash_str("pixel")],
	        },
	        shader_effect_configurations[hash_str("opaque_mesh")],
	        graph_set_layout,
	        {},
	        "opaque_mesh",
	    }
	);

	shader_pipelines.emplace(
	    hash_str("skybox"),

	    bvk::ShaderPipeline {
	        &vk_context,
	        &layout_allocator,
	        bvk::ShaderPipeline::Type::eGraphics,
	        {
	            &shaders[hash_str("skybox_vertex")],
	            &shaders[hash_str("skybox_fragment")],
	        },
	        shader_effect_configurations[hash_str("skybox")],
	        graph_set_layout,
	        {},
	        "skybox",
	    }
	);
}

void DevelopmentExampleApplication::load_compute_pipelines()
{
	auto const [bindings, flags] = BasicRendergraph::get_compute_descriptor_set_bindings();
	auto const graph_set_layout = layout_allocator.goc_descriptor_set_layout({}, bindings, flags);

	shader_pipelines.emplace(
	    hash_str("cull"),

	    bvk::ShaderPipeline {
	        &vk_context,
	        &layout_allocator,
	        bvk::ShaderPipeline::Type::eCompute,
	        {
	            &shaders[hash_str("cull")],
	        },
	        {},
	        graph_set_layout,
	        {},
	        "cull",
	    }
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
		    gpu.get_max_color_and_depth_samples(),
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
		    gpu.get_max_color_and_depth_samples(),
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
	    bvk::Material {
	        &descriptor_allocator,
	        &shader_pipelines.at(hash_str("opaque_mesh")),
	        descriptor_pool,
	    }
	);

	materials.emplace(
	    hash_str("skybox"),
	    bvk::Material {
	        &descriptor_allocator,
	        &shader_pipelines.at(hash_str("skybox")),
	        descriptor_pool,
	    }
	);
}

void DevelopmentExampleApplication::load_models()
{
	models.emplace(
	    hash_str("skybox"),
	    model_loader.load_from_gltf_ascii(
	        "Assets/Cube/Cube.gltf",
	        &vertex_buffer,
	        &index_buffer,
	        staging_pool.get_by_index(0u),
	        staging_pool.get_by_index(1u),
	        staging_pool.get_by_index(2u),
	        "skybox"
	    )
	);

	models.emplace(
	    hash_str("flight_helmet"),
	    model_loader.load_from_gltf_ascii(
	        "Assets/FlightHelmet/FlightHelmet.gltf",
	        &vertex_buffer,
	        &index_buffer,
	        staging_pool.get_by_index(0u),
	        staging_pool.get_by_index(1u),
	        staging_pool.get_by_index(2u),
	        "flight_helmet"
	    )
	);
}

// @todo: Load from files instead of hard-coding
void DevelopmentExampleApplication::load_entities()
{
	auto const test_model_entity = scene.create();

	scene.emplace<TransformComponent>(
	    test_model_entity,
	    glm::vec3(10.0, 200.0, -5.0),
	    glm::vec3(30.0),
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
	    glm::vec3(50.0f, 50.0f, 1.0f),
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
	    1000.0f,
	    225.0,
	    0.0,
	    glm::vec3(0.0f, 0.0f, -1.0f),
	    glm::vec3(0.0f, -1.0f, 0.0f),
	    10.0f
	);

	scene.emplace<LightComponent>(light_entity, 12);
}

auto DevelopmentExampleApplication::create_forward_pass_blueprint() -> bvk::RenderNodeBlueprint
{
	auto const color_format = surface.get_color_format();
	auto const depth_format = surface.get_depth_format();
	auto const sample_count = gpu.get_max_color_and_depth_samples();

	auto const color_output_hash = hash_str("forward_color_out");
	auto const depth_attachment_hash = hash_str("forward_depth");

	fowardpass_user_data = {
		&scene,
		&memory_allocator,
		&shader_pipelines[hash_str("opaque_mesh")],
		&shader_pipelines[hash_str("skybox")],
		&shader_pipelines[hash_str("cull")],
	};

	auto blueprint = bvk::RenderNodeBlueprint {};
	blueprint.set_derived_object(&forward_pass)
	    .set_user_data(&fowardpass_user_data)

	    .set_name("forwardpass")
	    .set_prepare_label(Forwardpass::get_prepare_label())
	    .set_compute_label(Forwardpass::get_compute_label())
	    .set_graphics_label(Forwardpass::get_graphics_label())
	    .set_barrier_label(Forwardpass::get_barrier_label())

	    .set_compute(true)
	    .set_graphics(true)

	    .set_sample_count(sample_count)

	    .add_color_output({
	        color_output_hash,
	        {},
	        {},
	        { 1.0, 1.0 },
	        bvk::RenderNode::SizeType::eSwapchainRelative,
	        color_format,
	        vk::ClearColorValue { 0.3f, 0.5f, 0.8f, 1.0f },
	        "forwardpass_depth",
	    })

	    .set_depth_attachment({
	        depth_attachment_hash,
	        {},
	        {},
	        { 1.0, 1.0 },
	        bvk::RenderNode::SizeType::eSwapchainRelative,
	        depth_format,
	        vk::ClearDepthStencilValue { 1.0, 0 },
	        "forwardpass_depth",
	    });

	return blueprint;
}

auto DevelopmentExampleApplication::create_ui_pass_blueprint() -> bvk::RenderNodeBlueprint
{
	auto const color_format = surface.get_color_format();
	auto const sample_count = gpu.get_max_color_and_depth_samples();

	auto const color_output_hash = hash_str("uipass_color_out");
	auto const color_output_input_hash = hash_str("forward_color_out");

	auto blueprint = bvk::RenderNodeBlueprint {};

	blueprint.set_derived_object(&user_interface_pass)
	    .set_user_data(&scene)

	    .set_name("uipass")
	    .set_prepare_label(UserInterfacePass::get_prepare_label())
	    .set_compute_label(UserInterfacePass::get_compute_label())
	    .set_graphics_label(UserInterfacePass::get_graphics_label())
	    .set_barrier_label(UserInterfacePass::get_barrier_label())

	    .set_compute(false)
	    .set_graphics(true)

	    .set_sample_count(sample_count)

	    .add_color_output({
	        color_output_hash,
	        color_output_input_hash,
	        {},
	        { 1.0, 1.0 },
	        bvk::RenderNode::SizeType::eSwapchainRelative,
	        color_format,
	        {},
	        "uipass_color",
	    });

	return blueprint;
}

auto DevelopmentExampleApplication::create_render_graph_blueprint() -> bvk::RenderNodeBlueprint
{
	auto blueprint = bvk::RenderNodeBlueprint {};

	auto const color_format = surface.get_color_format();
	auto const sample_count = gpu.get_max_color_and_depth_samples();

	auto const color_output_hash = hash_str("graph_color_out");
	auto const color_output_input_hash = hash_str("uipass_color_out");

	graph_user_data.scene = &scene;
	graph_user_data.vertex_buffer = &vertex_buffer;
	graph_user_data.index_buffer = &index_buffer;
	graph_user_data.memory_allocator = &memory_allocator;
	graph_user_data.debug_utils = &debug_utils;

	blueprint.set_derived_object(&render_graph)
	    .set_user_data(std::make_any<BasicRendergraph::UserData *>(&graph_user_data))

	    .set_compute(false)
	    .set_graphics(false)

	    .set_name("render_graph")
	    .set_compute_label(BasicRendergraph::get_compute_label())
	    .set_barrier_label(BasicRendergraph::get_barrier_label())
	    .set_graphics_label(BasicRendergraph::get_graphics_label())

	    .add_color_output({
	        color_output_hash,
	        color_output_input_hash,
	        {},
	        { 1.0, 1.0 },
	        bvk::RenderNode::SizeType::eSwapchainRelative,
	        color_format,
	        {},
	        "graph_color",
	    })

	    .set_sample_count(sample_count)

	    .add_buffer_input({
	        BasicRendergraph::U_FrameData::name,
	        BasicRendergraph::U_FrameData::key,
	        sizeof(BasicRendergraph::U_FrameData),
	        vk::BufferUsageFlagBits::eUniformBuffer,
	        bvk::RenderNodeBlueprint::BufferInput::UpdateFrequency::ePerFrame,
	        vma::AllocationCreateInfo {
	            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
	            vma::MemoryUsage::eAutoPreferDevice,
	        },
	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eCompute,
	                {
	                    BasicRendergraph::U_FrameData::binding,
	                    vk::DescriptorType::eUniformBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eCompute,
	                },
	            },
	            {

	                vk::PipelineBindPoint::eGraphics,
	                {
	                    BasicRendergraph::U_FrameData::binding,
	                    vk::DescriptorType::eUniformBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eVertex,
	                },
	            },
	        },
	    })

	    .add_buffer_input({
	        BasicRendergraph::U_SceneData::name,
	        BasicRendergraph::U_SceneData::key,
	        sizeof(BasicRendergraph::U_SceneData),
	        vk::BufferUsageFlagBits::eUniformBuffer,
	        bvk::RenderNodeBlueprint::BufferInput::UpdateFrequency::ePerFrame,
	        vma::AllocationCreateInfo {
	            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
	            vma::MemoryUsage::eAutoPreferDevice,
	        },
	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eCompute,
	                {
	                    BasicRendergraph::U_SceneData::binding,
	                    vk::DescriptorType::eUniformBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eCompute,
	                },
	            },
	            {
	                vk::PipelineBindPoint::eGraphics,
	                {
	                    BasicRendergraph::U_SceneData::binding,
	                    vk::DescriptorType::eUniformBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eVertex,
	                },
	            },
	        },
	    })

	    .add_buffer_input({
	        BasicRendergraph::U_ModelData::name,
	        BasicRendergraph::U_ModelData::key,
	        sizeof(BasicRendergraph::U_ModelData) * 1'000,
	        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
	        bvk::RenderNodeBlueprint::BufferInput::UpdateFrequency::eSingular,
	        vma::AllocationCreateInfo {
	            {},
	            vma::MemoryUsage::eAutoPreferDevice,
	        },
	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eCompute,
	                {
	                    BasicRendergraph::U_ModelData::binding,
	                    vk::DescriptorType::eStorageBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eCompute,
	                },
	            },
	            {
	                vk::PipelineBindPoint::eGraphics,
	                {
	                    BasicRendergraph::U_ModelData::binding,
	                    vk::DescriptorType::eStorageBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
	                },
	            },
	        },
	    })

	    .add_buffer_input({
	        BasicRendergraph::U_DrawIndirect::name,
	        BasicRendergraph::U_DrawIndirect::key,
	        sizeof(BasicRendergraph::U_DrawIndirect) * 6 * 3,

	        vk::BufferUsageFlagBits::eTransferDst         //
	            | vk::BufferUsageFlagBits::eTransferSrc   //
	            | vk::BufferUsageFlagBits::eStorageBuffer //
	            | vk::BufferUsageFlagBits::eIndirectBuffer,

	        bvk::RenderNodeBlueprint::BufferInput::UpdateFrequency::ePerFrame,

	        vma::AllocationCreateInfo {
	            {},
	            vma::MemoryUsage::eAutoPreferDevice,
	        },

	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eCompute,
	                vk::DescriptorSetLayoutBinding {
	                    BasicRendergraph::U_DrawIndirect::binding,
	                    vk::DescriptorType::eStorageBuffer,
	                    1,
	                    vk::ShaderStageFlagBits::eCompute,
	                },
	            },
	        },
	    })

	    .add_texture_input({
	        BasicRendergraph::U_Textures::name,
	        &textures.at(hash_str("default_texture")),
	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eGraphics,
	                vk::DescriptorSetLayoutBinding {
	                    BasicRendergraph::U_Textures::binding,
	                    vk::DescriptorType::eCombinedImageSampler,
	                    10'000,
	                    vk::ShaderStageFlagBits::eFragment,
	                },
	            },
	        },
	    })

	    .add_texture_input({
	        BasicRendergraph::U_TextureCubes::name,
	        &textures.at(hash_str("default_texture_cube")),
	        vec<bvk::RenderNodeBlueprint::DescriptorInfo> {
	            {
	                vk::PipelineBindPoint::eGraphics,
	                vk::DescriptorSetLayoutBinding {
	                    BasicRendergraph::U_TextureCubes::binding,
	                    vk::DescriptorType::eCombinedImageSampler,
	                    1'000,
	                    vk::ShaderStageFlagBits::eFragment,
	                },
	            },
	        },
	    })
	    .push_node(create_forward_pass_blueprint())
	    .push_node(create_ui_pass_blueprint());

	return blueprint;
}

void DevelopmentExampleApplication::initialize_render_nodes()
{
	render_graph = BasicRendergraph { &vk_context };
	forward_pass = Forwardpass { &vk_context };
	user_interface_pass = UserInterfacePass { &vk_context };
}

void DevelopmentExampleApplication::create_render_graph()
{
	auto builder = bvk::RenderGraphBuilder {
		&vk_context,
		&memory_allocator,
		&descriptor_allocator,
	};

	builder //
	    .set_resources(renderer.get_resources())
	    .push_node(create_render_graph_blueprint());

	builder.build_graph();
}
