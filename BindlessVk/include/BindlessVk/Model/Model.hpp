#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Texture/Texture.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

class Model
{
public:
	friend class ModelLoader;
	friend class GltfLoader;

public:
	struct Vertex
	{
		Vec3f position;
		Vec3f normal;
		Vec3f tangent;
		Vec2f uv;

		auto static get_attributes() -> arr<vk::VertexInputAttributeDescription, 4>;
		auto static get_bindings() -> arr<vk::VertexInputBindingDescription, 1>;
		auto static get_vertex_input_state() -> vk::PipelineVertexInputStateCreateInfo;
	};

	struct Node
	{
		struct Primitive
		{
			u32 first_index;
			u32 index_count;
			i32 material_index;
		};

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
		Mat4f transform;
	};

	struct MaterialParameters
	{
		Vec3f albedo = Vec3f(1.0f);
		Vec3f diffuse = Vec3f(1.0f);
		Vec3f specular = Vec3f(1.0f);

		i32 albedo_texture_index;
		i32 normal_texture_index;
		i32 metallic_roughness_texture_index;
	};

public:
	~Model();

	Model(Model &&);
	Model &operator=(Model &&);

	Model(const Model &) = delete;
	Model &operator=(const Model &) = delete;

	auto get_name() const
	{
		return str_view(debug_name);
	}

	auto &get_nodes() const
	{
		return nodes;
	}

	auto &get_textures() const
	{
		return textures;
	}

	auto &get_material_parameters() const
	{
		return material_parameters;
	}

	auto get_vertex_buffer() const
	{
		return vertex_buffer;
	}

	auto get_index_buffer() const
	{
		return index_buffer;
	}

private:
	Model() = default;

private:
	vec<Node *> nodes = {};
	vec<Texture> textures = {};
	vec<MaterialParameters> material_parameters = {};

	class Buffer *vertex_buffer = {};
	class Buffer *index_buffer = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE
