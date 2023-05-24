#include "Rendergraphs/Graph.hpp"

#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

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
	debug_util = data->debug_utils;

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

	upload_model_data_to_gpu();
	upload_indirect_data_to_gpu();
}

void BasicRendergraph::upload_model_data_to_gpu()
{
	auto &model_buffer = buffer_inputs[U_ModelData::key];

	auto *const map = reinterpret_cast<U_ModelData *>(staging_buffer.map_block_zeroed(0));

	for (auto i = u32 { 0 }; i < bvk::max_frames_in_flight; ++i)
		stage_model(map, i);

	staging_buffer.unmap();

	debug_util->log(bvk::LogLvl::eInfo, "Primitive count: {}", primitive_count);
	model_buffer.write_buffer(
	    staging_buffer,
	    vk::BufferCopy {
	        0u,
	        0,
	        sizeof(U_ModelData) * primitive_count,
	    }
	);
}

void BasicRendergraph::upload_indirect_data_to_gpu()
{
	auto &indirect_buffer = buffer_inputs[U_DrawIndirect::key];

	auto *const map = static_cast<U_DrawIndirect *>(staging_buffer.map_block_zeroed(0));

	stage_indirect(map);
	staging_buffer.unmap();

	for (u32 i = 0; i < bvk::max_frames_in_flight; ++i)
	{
		indirect_buffer.write_buffer(
		    staging_buffer,
		    vk::BufferCopy {
		        0u,
		        indirect_buffer.get_block_size() * i,
		        sizeof(U_DrawIndirect) * primitive_count,
		    }
		);
	}
}

void BasicRendergraph::stage_indirect(U_DrawIndirect *buffer_map)
{
	auto i = u32 { 0 };
	auto const static_meshes = scene->view<
	    TransformComponent const,
	    StaticMeshRendererComponent const>();

	static_meshes.each([this, &i, buffer_map](auto const &transform, auto const &static_mesh) {
		auto const *model = static_mesh.model;

		auto const vertex_offset = model->get_vertex_offset();
		auto const index_offset = model->get_index_offset();

		for (auto const *node : model->get_nodes())
			for (auto const &primitive : node->mesh)
			{
				buffer_map[i].cmd = vk::DrawIndexedIndirectCommand {
					primitive.index_count, //
					1,
					primitive.first_index + index_offset,
					vertex_offset,
					i,
				};
				++i;
			}
	});
}

void BasicRendergraph::on_frame_prepare(u32 const frame_index, u32 const image_index)
{
	this->frame_index = frame_index;

	// These calls can accumulate descriptor_writes
	update_for_cameras();
	update_for_lights();
	update_for_skyboxes();

	update_descriptor_sets();
}

void BasicRendergraph::on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
	bind_graphic_buffers(cmd);
}

void BasicRendergraph::bind_graphic_buffers(vk::CommandBuffer const cmd)
{
	vertex_buffer->bind(cmd);
	index_buffer->bind(cmd);
}

void BasicRendergraph::update_descriptor_sets()
{
	if (descriptor_writes.empty())
		return;

	device->vk().updateDescriptorSets(descriptor_writes, {});
	descriptor_writes.clear();
}

void BasicRendergraph::update_for_cameras()
{
	auto const cameras = scene->view<TransformComponent const, CameraComponent const>();
	cameras.each([&](auto const &transform, auto const &camera) {
		update_for_camera(transform, camera);
	});
}

void BasicRendergraph::update_for_lights()
{
	auto const lights = scene->view<TransformComponent const, LightComponent const>();
	lights.each([this](auto const &transform, auto const &light) {
		update_for_light(transform, light);
	});
}

void BasicRendergraph::stage_model(U_ModelData *const buffer_map, u32 buffer_index)
{
	auto i = u32 { 0 };
	auto const static_meshes = scene->view<
	    TransformComponent const,
	    StaticMeshRendererComponent const>();

	static_meshes.each(
	    [this, &i, buffer_map, buffer_index](auto const &transform, auto const &static_mesh) {
		    update_for_model(buffer_map, buffer_index, transform, static_mesh, i);
	    }
	);
}

void BasicRendergraph::update_for_skyboxes()
{
	auto const sky_boxes = scene->view<SkyboxComponent const>();
	sky_boxes.each([this](auto const &skybox) { update_for_skybox(skybox); });
}

void BasicRendergraph::update_for_camera(
    TransformComponent const &transform,
    CameraComponent const &camera
)
{
	auto &frame_data_buffer = buffer_inputs[U_FrameData::key];
	auto *const frame_data = static_cast<U_FrameData *>(frame_data_buffer.map_block(frame_index));

	*frame_data = U_FrameData {
		camera.get_view(transform.translation),
		camera.get_projection(),
		glm::vec4(transform.translation, 1.0f),
	};

	frame_data_buffer.unmap();
}

void BasicRendergraph::update_for_light(
    TransformComponent const &transform,
    LightComponent const &camera
)
{
}

void BasicRendergraph::update_for_model(
    U_ModelData *const buffer_map,
    u32 buffer_index,
    TransformComponent const &transform,
    StaticMeshRendererComponent const &static_mesh,
    u32 &primitive_index
)
{
	auto const &descriptor_set = graphics_descriptor_sets[buffer_index];

	for (auto const *const node : static_mesh.model->get_nodes())
		for (auto const &primitive : node->mesh)
		{
			++primitive_count;
			auto &object = buffer_map[primitive_index++];
			object.model = transform.get_transform();

			auto const &model = static_mesh.model;
			auto const &material = model->get_material_parameters()[primitive.material_index];
			auto const &textures = model->get_textures();

			auto const albedo_index = material.albedo_texture_index;
			auto const normal_index = material.normal_texture_index;
			auto const metallic_roughness_index = material.metallic_roughness_texture_index;

			object.x = transform.translation.x;
			object.y = transform.translation.y;
			object.z = transform.translation.z;
			object.r = std::max(transform.scale.x, std::max(transform.scale.y, transform.scale.z));

			if (albedo_index != -1)
			{
				descriptor_writes.push_back({
				    descriptor_set.vk(),
				    U_Textures::binding,
				    static_cast<u32>(albedo_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[albedo_index].get_descriptor_info(),
				});

				object.albedo_texture_index = albedo_index;
			}

			if (metallic_roughness_index != -1)
			{
				descriptor_writes.push_back({
				    descriptor_set.vk(),
				    U_Textures::binding,
				    static_cast<u32>(metallic_roughness_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[metallic_roughness_index].get_descriptor_info(),
				});

				object.metallic_roughness_texture_index = metallic_roughness_index;
			}

			if (normal_index != -1)
			{
				descriptor_writes.push_back({
				    descriptor_set.vk(),
				    U_Textures::binding,
				    static_cast<u32>(normal_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[normal_index].get_descriptor_info(),
				});

				object.normal_texture_index = normal_index;
			}
		}
}

void BasicRendergraph::update_for_skybox(SkyboxComponent const &skybox)
{
	auto const &descriptor_set = graphics_descriptor_sets[frame_index];

	descriptor_writes.push_back({
	    descriptor_set.vk(),
	    U_TextureCubes::binding,
	    0,
	    1,
	    vk::DescriptorType::eCombinedImageSampler,
	    skybox.texture->get_descriptor_info(),
	});
}

auto BasicRendergraph::get_graphics_descriptor_set_bindings() -> pair<
    arr<vk::DescriptorSetLayoutBinding, graphics_descriptor_set_bindings_count>,
    arr<vk::DescriptorBindingFlags, graphics_descriptor_set_bindings_count>>
{
	return {
		arr<vk::DescriptorSetLayoutBinding, graphics_descriptor_set_bindings_count> {
		    vk::DescriptorSetLayoutBinding {
		        U_FrameData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },
		    vk::DescriptorSetLayoutBinding {
		        U_SceneData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },

		    vk::DescriptorSetLayoutBinding {
		        U_ModelData::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		    },

		    vk::DescriptorSetLayoutBinding {
		        U_Textures::binding,
		        vk::DescriptorType::eCombinedImageSampler,
		        10'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },

		    vk::DescriptorSetLayoutBinding {
		        U_TextureCubes::binding,
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
		        U_FrameData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },
		    vk::DescriptorSetLayoutBinding {
		        U_SceneData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },

		    vk::DescriptorSetLayoutBinding {
		        U_ModelData::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },

		    vk::DescriptorSetLayoutBinding {
		        U_DrawIndirect::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eCompute,
		    },
		},

		arr<vk::DescriptorBindingFlags, compute_descriptor_set_bindings_count> {
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		},
	};
}
