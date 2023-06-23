#include "Rendergraphs/Graph.hpp"

#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

#include <imgui.h>

BasicRendergraph::BasicRendergraph(bvk::VkContext const *const vk_context)
    : bvk::RenderNode(vk_context)
    , device(vk_context->get_device())
{
}

void BasicRendergraph::on_setup(RenderNode *const parent)
{
	auto data = std::any_cast<UserData *>(user_data);
	scene = data->scene;
	vertex_buffer = data->vertex_buffer;
	index_buffer = data->index_buffer;

	staging_buffer = bvk::Buffer {
		vk_context,
		data->memory_allocator,
		vk::BufferUsageFlagBits::eTransferSrc,
		vma::AllocationCreateInfo {
		    vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
		    vma::MemoryUsage::eAutoPreferHost,
		},
		1024 * 1024 * 1024,
		1,
		"graph_staging_buffer",
	};

	setup_descriptors();
}

void BasicRendergraph::setup_descriptors()
{
	setup_descriptor_maps();
	setup_primitives_descriptor();
	setup_draw_indirects_descriptor();
}

void BasicRendergraph::setup_descriptor_maps()
{
	staging_buffer_map = staging_buffer.map_block_zeroed(0);
	setup_frame_descriptor_maps();
}

void BasicRendergraph::setup_frame_descriptor_maps()
{
	auto maps = buffer_inputs[FrameDescriptor::key].map_all_zeroed();

	u32 i = 0;
	for (auto map : maps)
		frame_descriptor_maps[i++] = static_cast<FrameDescriptor *>(map);
}

void BasicRendergraph::setup_primitives_descriptor()
{
	auto &model_buffer = buffer_inputs[PrimitivesDescriptor::key];

	for (auto i = u32 { 0 }; i < bvk::max_frames_in_flight; ++i)
		stage_static_meshes(i);

	log_inf("Primitive count: {}", primitive_count);
	model_buffer.write_buffer(
	    staging_buffer,
	    vk::BufferCopy {
	        0u,
	        0,
	        sizeof(PrimitivesDescriptor) * primitive_count,
	    }
	);
}

void BasicRendergraph::setup_draw_indirects_descriptor()
{
	stage_indirect();

	auto &indirect_buffer = buffer_inputs[DrawIndirectDescriptor::key];
	for (u32 i = 0; i < bvk::max_frames_in_flight; ++i)
		indirect_buffer.write_buffer(
		    staging_buffer,
		    vk::BufferCopy {
		        0u,
		        indirect_buffer.get_block_size() * i,
		        sizeof(DrawIndirectDescriptor) * primitive_count,
		    }
		);

	memset(staging_buffer_map, {}, staging_buffer.get_whole_size());
}

void BasicRendergraph::stage_static_meshes(u32 buffer_index)
{
	auto i = u32 { 0 };
	auto const static_meshes = scene->view<TransformComponent const, StaticMeshComponent const>();

	static_meshes.each([this, &i, buffer_index](auto const &transform, auto const &static_mesh) {
		stage_static_mesh(buffer_index, transform, static_mesh, i);
	});
}

void BasicRendergraph::stage_indirect()
{
	auto *const map = static_cast<DrawIndirectDescriptor *>(staging_buffer_map);

	auto i = u32 { 0 };
	auto const static_meshes = scene->view<TransformComponent const, StaticMeshComponent const>();

	static_meshes.each([this, &i, map](auto const &transform, auto const &static_mesh) {
		auto const *model = static_mesh.model;

		auto const vertex_offset = model->get_vertex_offset();
		auto const index_offset = model->get_index_offset();

		for (auto const *node : model->get_nodes())
			for (auto const &primitive : node->mesh)
			{
				map[i].cmd = vk::DrawIndexedIndirectCommand {
					primitive.index_count, //
					0,
					primitive.first_index + index_offset,
					vertex_offset,
					i,
				};

				++i;
			}
	});
}

void BasicRendergraph::update_delta_time()
{
	frame_descriptor_maps[frame_index]->delta_time = timer.elapsed_time();
	timer.reset();
}

void BasicRendergraph::on_frame_prepare(u32 const frame_index, u32 const image_index)
{
	this->frame_index = frame_index;

	update_frame();
	update_descriptors();
}

void BasicRendergraph::on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
}

void BasicRendergraph::on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
	bind_graphics_buffers(cmd);
}

void BasicRendergraph::update_frame()
{
	// These calls may accumulate descriptr writes
	update_delta_time();

	update_cameras();

	update_skyboxes();

	update_directional_lights();
	update_point_lights();

	frame_descriptor_maps[frame_index]->render_scene.primitive_count = primitive_count;
}

void BasicRendergraph::bind_graphics_buffers(vk::CommandBuffer const cmd)
{
	vertex_buffer->bind(cmd);
	index_buffer->bind(cmd);
}

void BasicRendergraph::update_descriptors()
{
	if (descriptor_writes.empty())
		return;

	device->vk().updateDescriptorSets(descriptor_writes, {});
	descriptor_writes.clear();
}

void BasicRendergraph::update_cameras()
{
	auto const cameras = scene->view<TransformComponent const, CameraComponent const>();
	cameras.each([&](auto const &transform, auto const &camera) {
		update_camera(transform, camera);
	});
}

void BasicRendergraph::update_skyboxes()
{
	auto const skyboxes = scene->view<SkyboxComponent const>();
	skyboxes.each([this](auto const &skybox) { update_skybox(skybox); });
}

void BasicRendergraph::update_directional_lights()
{
	auto const directional_lights = scene->view<DirectionalLightComponent const>();
	directional_lights.each([this](auto const directional_light) {
		update_directional_light(directional_light);
	});
}

void BasicRendergraph::update_point_lights()
{
	auto const lights = scene->view<TransformComponent const, PointLightComponent const>();
	u32 index = 0;

	ImGui::DragFloat("linear", &linear, 0.002f, 0.0014, 0.7);
	ImGui::DragFloat("quadratic", &quadratic, 0.002f, 0.000007, 1.8);

	lights.each([this, &index](auto const &transform, auto const &light) {
		update_point_light(transform, light, index++);
	});

	frame_descriptor_maps[frame_index]->render_scene.point_light_count = 6;
}

void BasicRendergraph::update_camera(
    TransformComponent const &transform,
    CameraComponent const &camera
)
{
	frame_descriptor_maps[frame_index]->camera = Camera {
		camera.get_view(transform.translation),
		camera.get_projection(),
		camera.get_view_projection(transform.translation),
		glm::vec4(transform.translation, 1.0)
	};
};

void BasicRendergraph::update_skybox(SkyboxComponent const &skybox)
{
	auto const &descriptor_set = graphics_descriptor_sets[frame_index];

	descriptor_writes.push_back({
	    descriptor_set.vk(),
	    TextureCubesDescriptor::binding,
	    0,
	    1,
	    vk::DescriptorType::eCombinedImageSampler,
	    skybox.texture->get_descriptor_info(),
	});
}

void BasicRendergraph::update_directional_light(DirectionalLightComponent const &directional_light)
{
	auto &render_scene = frame_descriptor_maps[frame_index]->render_scene;
	render_scene.directional_light = DirectionalLight {
		directional_light.direction,
		directional_light.ambient,
		directional_light.diffuse,
		directional_light.specular,
	};
}

void BasicRendergraph::update_point_light(
    TransformComponent const &transform,
    PointLightComponent const &point_light,
    u32 const index
)
{
	if (index > PointLight::max_count)
	{
		log_wrn("Point lights exceed max count: index({})> max({})", index, PointLight::max_count);

		return;
	}

	auto &render_scene = frame_descriptor_maps[frame_index]->render_scene;
	render_scene.point_lights[index] = PointLight {
		glm::vec4(transform.translation.x, transform.translation.y, transform.translation.z, 0.0),

		point_light.constant, //
		linear,               //
		quadratic,            //
		{},

		point_light.ambient, //
		point_light.diffuse, //
		point_light.specular,
	};
}
void BasicRendergraph::update_primitive_textures(
    bvk::DescriptorSet const &descriptor_set,
    vec<bvk::Texture> const &textures,
    bvk::Model::MaterialParameters material
)
{
	if (material.albedo_index != -1)
		descriptor_writes.push_back({
		    descriptor_set.vk(),
		    TexturesDescriptor::binding,
		    static_cast<u32>(material.albedo_index),
		    1,
		    vk::DescriptorType::eCombinedImageSampler,
		    textures[material.albedo_index].get_descriptor_info(),
		});

	if (material.mr_index != -1)
		descriptor_writes.push_back({
		    descriptor_set.vk(),
		    TexturesDescriptor::binding,
		    static_cast<u32>(material.mr_index),
		    1,
		    vk::DescriptorType::eCombinedImageSampler,
		    textures[material.mr_index].get_descriptor_info(),
		});

	if (material.normal_index != -1)
		descriptor_writes.push_back({
		    descriptor_set.vk(),
		    TexturesDescriptor::binding,
		    static_cast<u32>(material.normal_index),
		    1,
		    vk::DescriptorType::eCombinedImageSampler,
		    textures[material.normal_index].get_descriptor_info(),
		});
}

void BasicRendergraph::update_primitive_buffer(
    PrimitivesDescriptor &primitive,
    TransformComponent const &transform,
    bvk::Model::MaterialParameters const &material
)
{
	primitive.transform = transform.get_transform();

	primitive.x = transform.translation.x;
	primitive.y = transform.translation.y;
	primitive.z = transform.translation.z;

	primitive.r = std::max(transform.scale.x, std::max(transform.scale.y, transform.scale.z));

	primitive.albedo_index = material.albedo_index;
	primitive.mr_index = material.mr_index;
	primitive.normal_index = material.normal_index;
}

void BasicRendergraph::stage_static_mesh(
    u32 const buffer_index,
    TransformComponent const &transform,
    StaticMeshComponent const &static_mesh,
    u32 &primitive_index
)
{
	auto *const map = static_cast<PrimitivesDescriptor *>(staging_buffer_map);
	auto const &descriptor_set = graphics_descriptor_sets[buffer_index];

	auto const &textures = static_mesh.model->get_textures();
	auto const &materials = static_mesh.model->get_material_parameters();

	for (auto const *const node : static_mesh.model->get_nodes())
		for (auto const &primitive : node->mesh)
		{
			++primitive_count;

			auto const &material = materials[primitive.material_index];

			update_primitive_buffer(map[primitive_index++], transform, material);
			update_primitive_textures(descriptor_set, textures, material);
		}
}

auto BasicRendergraph::get_graphics_descriptor_set_bindings() -> pair<
    arr<vk::DescriptorSetLayoutBinding, graphics_descriptor_set_bindings_count>,
    arr<vk::DescriptorBindingFlags, graphics_descriptor_set_bindings_count>>
{
	return {
		arr<vk::DescriptorSetLayoutBinding, graphics_descriptor_set_bindings_count> {
		    vk::DescriptorSetLayoutBinding {
		        FrameDescriptor::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		    },

		    vk::DescriptorSetLayoutBinding {
		        PrimitivesDescriptor::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		    },

		    vk::DescriptorSetLayoutBinding {
		        TexturesDescriptor::binding,
		        vk::DescriptorType::eCombinedImageSampler,
		        10'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },

		    vk::DescriptorSetLayoutBinding {
		        TextureCubesDescriptor::binding,
		        vk::DescriptorType::eCombinedImageSampler,
		        1'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },
		},

		arr<vk::DescriptorBindingFlags, graphics_descriptor_set_bindings_count> {
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		},
	};
}

auto BasicRendergraph::get_compute_descriptor_set_bindings() -> pair<
    arr<vk::DescriptorSetLayoutBinding, compute_descriptor_set_bindings_count>,
    arr<vk::DescriptorBindingFlags, compute_descriptor_set_bindings_count>>
{
	return {
		arr<vk::DescriptorSetLayoutBinding, compute_descriptor_set_bindings_count> {
		    vk::DescriptorSetLayoutBinding {
		        FrameDescriptor::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },

		    vk::DescriptorSetLayoutBinding {
		        PrimitivesDescriptor::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },

		    vk::DescriptorSetLayoutBinding {
		        DrawIndirectDescriptor::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },
		},

		arr<vk::DescriptorBindingFlags, compute_descriptor_set_bindings_count> {
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		},
	};
}
