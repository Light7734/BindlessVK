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
    -> pair<arr<vk::DescriptorSetLayoutBinding, 5>, arr<vk::DescriptorBindingFlags, 5>>
{
	return pair<arr<vk::DescriptorSetLayoutBinding, 5>, arr<vk::DescriptorBindingFlags, 5>> {
		arr<vk::DescriptorSetLayoutBinding, 5> {
		    vk::DescriptorSetLayoutBinding {
		        FrameData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },
		    vk::DescriptorSetLayoutBinding {
		        SceneData::binding,
		        vk::DescriptorType::eUniformBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex,
		    },
		    vk::DescriptorSetLayoutBinding {
		        ObjectData::binding,
		        vk::DescriptorType::eStorageBuffer,
		        1,
		        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		    },
		    vk::DescriptorSetLayoutBinding {
		        3,
		        vk::DescriptorType::eCombinedImageSampler,
		        10'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },
		    vk::DescriptorSetLayoutBinding {
		        4,
		        vk::DescriptorType::eCombinedImageSampler,
		        1'000,
		        vk::ShaderStageFlagBits::eFragment,
		    },
		},

		arr<vk::DescriptorBindingFlags, 5> {
		    vk::DescriptorBindingFlagBits::ePartiallyBound,
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
	test();
	update_for_cameras();
	update_for_lights();
	update_for_meshes();
	update_for_skyboxes();

	update_descriptor_sets();
}

void BasicRendergraph::test()
{
	auto const static_meshes = scene->view<TransformComponent, StaticMeshRendererComponent const>();

	static_meshes.each([this](auto &transform, auto const &mesh) {
		// transform.rotation.y += 0.05;
	});
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
	auto i = u32 { 0 };
	auto const static_meshes = scene->view<
	    TransformComponent const,
	    StaticMeshRendererComponent const>();

	static_meshes.each([this, &i](auto const &transform, auto const &static_mesh) {
		update_for_mesh(transform, static_mesh, i);
	});
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

void BasicRendergraph::update_for_mesh(
    TransformComponent const &transform,
    StaticMeshRendererComponent const &static_mesh,
    u32 &primitive_index
)
{
	auto &object_data_buffer = buffer_inputs[ObjectData::binding];
	auto *const object_data = reinterpret_cast<ObjectData *>(
	    object_data_buffer.map_block(frame_index)
	);


	auto const descriptor_set = descriptor_sets[frame_index];

	for (auto const *const node : static_mesh.model->get_nodes())
		for (auto const &primitive : node->mesh)
		{
			auto &object = object_data[primitive_index++];
			object.model = transform.get_transform();

			auto const &model = static_mesh.model;
			auto const &material = model->get_material_parameters()[primitive.material_index];
			auto const &textures = model->get_textures();

			auto const albedo_index = material.albedo_texture_index;
			auto const normal_index = material.normal_texture_index;
			auto const metallic_roughness_index = material.metallic_roughness_texture_index;

			if (albedo_index != -1)
			{
				descriptor_writes.emplace_back(
				    descriptor_set,
				    U_Textures::binding,
				    albedo_index,
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[albedo_index].get_descriptor_info()
				);

				object.albedo_texture_index = albedo_index;
			}

			if (metallic_roughness_index != -1)
			{
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				    descriptor_set,
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
				descriptor_writes.emplace_back(vk::WriteDescriptorSet {
				    descriptor_set,
				    U_Textures::binding,
				    static_cast<u32>(normal_index),
				    1,
				    vk::DescriptorType::eCombinedImageSampler,
				    textures[normal_index].get_descriptor_info(),
				});

				object.normal_texture_index = normal_index;
			}
		}

	object_data_buffer.unmap();
}

void BasicRendergraph::update_for_skybox(SkyboxComponent const &skybox)
{
	auto const descriptor_set = descriptor_sets[frame_index];

	descriptor_writes.emplace_back(vk::WriteDescriptorSet {
	    descriptor_set,
	    U_TextureCubes::binding,
	    0,
	    1,
	    vk::DescriptorType::eCombinedImageSampler,
	    skybox.texture->get_descriptor_info(),
	});
}
