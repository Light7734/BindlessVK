#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Texture/Texture.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

struct Model
{
	struct Vertex
	{
		vec3 position;
		vec3 normal;
		vec3 tangent;
		vec2 uv;
		vec3 color;

		static constexpr std::array<vk::VertexInputBindingDescription, 1> get_bindings()
		{
			return {
				vk::VertexInputBindingDescription {
				    0u,                                      // binding
				    static_cast<u32>(sizeof(Model::Vertex)), // stride
				    vk::VertexInputRate::eVertex,            // inputRate
				},
			};
		}

		static constexpr std::array<vk::VertexInputAttributeDescription, 5> get_attributes()
		{
			return {
				vk::VertexInputAttributeDescription {
				    0u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, position),
				},
				vk::VertexInputAttributeDescription {
				    1u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, normal),
				},
				vk::VertexInputAttributeDescription {
				    2u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, tangent),
				},
				vk::VertexInputAttributeDescription {
				    3u,
				    0u,
				    vk::Format::eR32G32Sfloat,
				    offsetof(Model::Vertex, uv),
				},
				vk::VertexInputAttributeDescription {
				    4u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, color),
				},


			};
			offsetof(Model::Vertex, uv);
		}

		static vk::PipelineVertexInputStateCreateInfo get_vertex_input_state()
		{
			static const auto bindings = get_bindings();
			static const auto attributes = get_attributes();

			return vk::PipelineVertexInputStateCreateInfo {
				{},
				static_cast<u32>(bindings.size()),
				bindings.data(),

				static_cast<u32>(attributes.size()),
				attributes.data(),
			};
		}
	};

	struct Primitive
	{
		u32 first_index;
		u32 index_count;
		i32 material_index;
	};

	struct Node
	{
		Node(Node *parent): parent(parent)
		{
		}

		~Node()
		{
			for (auto *node : children)
				delete node;
		}

		Node *parent;
		vec<Node *> children;

		vec<Primitive> mesh;
		mat4f transform;
	};

	struct MaterialParameters
	{
		vec3 albedo_factor = vec3(1.0f);
		vec3 diffuse_factor = vec3(1.0f);
		vec3 specular_factor = vec3(1.0f);

		i32 albedo_texture_index;
		i32 normal_texture_index;
		i32 metallic_roughness_texture_index;
	};

	/// @todo: Support textures with different samplers
	str debug_name;
	vec<Texture> textures;
	vec<MaterialParameters> material_parameters;
	vec<Node *> nodes;

	class Buffer *vertex_buffer;
	class Buffer *index_buffer;
};

} // namespace BINDLESSVK_NAMESPACE
