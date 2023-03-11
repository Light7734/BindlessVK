#include "Rendergraphs/Graph.hpp"

#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

BasicRendergraph::BasicRendergraph(ref<bvk::VkContext> const vk_context)
    : bvk::Rendergraph(vk_context)
{
}

auto BasicRendergraph::get_descriptor_set_bindings()
    -> pair<arr<vk::DescriptorSetLayoutBinding, 4>, arr<vk::DescriptorBindingFlags, 4>>
{
	return pair<arr<vk::DescriptorSetLayoutBinding, 4>, arr<vk::DescriptorBindingFlags, 4>> {
		arr<vk::DescriptorSetLayoutBinding, 4> {
		    vk::DescriptorSetLayoutBinding {
		        0,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },
		    vk::DescriptorSetLayoutBinding {
		        1,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },
		    vk::DescriptorSetLayoutBinding {
		        2,
		        vk::DescriptorType::eCombinedImageSampler,
		        10'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },
		    vk::DescriptorSetLayoutBinding {
		        3,
		        vk::DescriptorType::eCombinedImageSampler,
		        1'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },
		},

		arr<vk::DescriptorBindingFlags, 4> {
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
		}
	};
}

void BasicRendergraph::on_update(u32 const frame_index, u32 const image_index)
{
	this->frame_index = frame_index;
	scene = any_cast<Scene *>(user_data);

	// These calls can accumulate descriptor_writes
	update_for_cameras();
	update_for_lights();
	update_for_meshes();
	update_for_skyboxes();

	update_descriptor_sets();
}

void BasicRendergraph::update_descriptor_sets()
{
	auto const device = vk_context->get_device();

	if (!descriptor_writes.empty())
	{
		device.updateDescriptorSets(descriptor_writes, {});
		descriptor_writes.clear();
	}
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

void BasicRendergraph::update_for_meshes()
{
	auto const static_meshes = scene->view<StaticMeshRendererComponent const>();
	static_meshes.each([this](auto const &static_mesh) { update_for_mesh(static_mesh); });
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
	auto &frame_data_buffer = buffer_inputs[FrameData::binding];
	auto *const frame_data = reinterpret_cast<FrameData *>(frame_data_buffer.map_block(frame_index)
	);

	*frame_data = FrameData {
		camera.get_projection(),
		camera.get_view(transform.translation),
		glm::vec4(transform.translation, 1.0f),
	};

	frame_data_buffer.unmap();
}

void BasicRendergraph::update_for_light(
    TransformComponent const &transform,
    LightComponent const &camera
)
{
	auto &scene_data_buffer = buffer_inputs[SceneData::binding];
	auto *const scene_data = reinterpret_cast<SceneData *>(scene_data_buffer.map_block(frame_index)
	);

	*scene_data = SceneData {
		glm::vec4(transform.translation, 1.0f),
	};

	scene_data_buffer.unmap();
}

void BasicRendergraph::update_for_mesh(StaticMeshRendererComponent const &static_mesh)
{
	auto const descriptor_set = descriptor_sets[frame_index];

	for (auto const *const node : static_mesh.model->get_nodes())
		for (auto const &primitive : node->mesh)
		{
			auto const &model = static_mesh.model;
			auto const &material = model->get_material_parameters()[primitive.material_index];
			auto const &textures = model->get_textures();

			auto const albedo_index = material.albedo_texture_index;
			auto const normal_index = material.normal_texture_index;
			auto const metallic_roughness_index = material.metallic_roughness_texture_index;

			if (albedo_index != -1)
				descriptor_writes.emplace_back(
				    descriptor_set,
				    2,
				    albedo_index,
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[albedo_index].get_descriptor_info()
				);

			if (metallic_roughness_index != -1)
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				    descriptor_set,
				    2,
				    static_cast<u32>(metallic_roughness_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[metallic_roughness_index].get_descriptor_info(),
				});

			if (normal_index != -1)
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				    descriptor_set,
				    2,
				    static_cast<u32>(normal_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[normal_index].get_descriptor_info(),
				});
		}
}

void BasicRendergraph::update_for_skybox(SkyboxComponent const &skybox)
{
	auto const descriptor_set = descriptor_sets[frame_index];

	descriptor_writes.emplace_back(vk::WriteDescriptorSet {
	    descriptor_set,
	    3,
	    0,
	    1,
	    vk::DescriptorType::eCombinedImageSampler,
	    skybox.texture->get_descriptor_info(),
	});
}
