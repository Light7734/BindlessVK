#pragma once

#include "BindlessVk/Buffers/FragmentedBuffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Texture/Texture.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

/** Holds 3d mesh data
 *
 * The constructor is private, you need to use the friended loader classes to constrruct models
 * */
class Model
{
public:
	friend class ModelLoader;
	friend class GltfLoader;

public:
	/** Holds attibutes of points-in-space to represent a geometric shape in gpu */
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

	/** A node that holds child-nodes and/or primitives and their transforrmation matrix */
	struct Node
	{
		/** Renderable primitive composed of a set of vertex indices and a material */
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

	/** Parameters that affect the shading of models */
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
	/** Default move constructor */
	Model(Model &&) = default;

	/** Default move assignment operator */
	Model &operator=(Model &&) = default;

	/** Deleted copy constructor */
	Model(const Model &) = delete;

	/** Deleted copy assignment operator */
	Model &operator=(const Model &) = delete;

	/** Destructor */
	~Model();

	/** Returns null-terminated str view to debug_name */
	auto get_name() const
	{
		return str_view(debug_name);
	}

	/** Trivial const-ref accessor for nodes */
	auto &get_nodes() const
	{
		return nodes;
	}

	/** Trivial const-ref accessor for textures */
	auto &get_textures() const
	{
		return textures;
	}

	/** Trivial const-ref accessor for material_parameters */
	auto &get_material_parameters() const
	{
		return material_parameters;
	}

	/**  Calcualtes the vertex offset from beginning of vertex buffer to model's first vertex */
	auto get_vertex_offset() const
	{
		return vertex_buffer_fragment.offset / sizeof(Model::Vertex);
	}

	/**  Calcualtes the index offset from beginning of index buffer to model's first index */
	auto get_index_offset() const
	{
		return index_buffer_fragment.offset / sizeof(u32);
	}

private:
	Model() = default;

private:
	vec<Node *> nodes = {};
	vec<Texture> textures = {};
	vec<MaterialParameters> material_parameters = {};

	FragmentedBuffer::Fragment vertex_buffer_fragment = {};
	FragmentedBuffer::Fragment index_buffer_fragment = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE
