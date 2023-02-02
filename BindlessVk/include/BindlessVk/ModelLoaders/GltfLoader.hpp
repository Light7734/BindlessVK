#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Model.hpp"
#include "BindlessVk/TextureLoader.hpp"
#include "BindlessVk/VkContext.hpp"

#include <glm/glm.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

/**
 * @todo Implement load_from_binary
 * @todo Implement parse_binary_file
 */
class GltfLoader
{
public:
	GltfLoader(
	    VkContext const *vk_context,
	    TextureLoader const *texture_loader,
	    Buffer *staging_vertex_buffer,
	    Buffer *staging_index_buffer,
	    Buffer *staging_texture_buffer
	);
	GltfLoader() = default;
	~GltfLoader() = default;

	auto load_from_ascii(c_str debug_name, c_str file_path) -> Model;
	auto load_from_binary(c_str debug_name, c_str file_path) -> Model = delete;

private:
	void load_gltf_model_from_ascii(c_str file_path);
	void load_gltf_model_from_binary(c_str file_path) = delete;

	void load_textures();
	void load_material_parameters();
	void stage_mesh_data();

	void write_mesh_data_to_gpu();

	auto load_node(tinygltf::Node const &gltf_node, Model::Node *parent_node) -> Model::Node *;

	void write_vertex_buffer_to_gpu();
	void write_index_buffer_to_gpu();

	void load_mesh_primitives(tinygltf::Mesh const &gltf_mesh, Model::Node *node);

	void load_mesh_primitive_vertices(tinygltf::Primitive const &gltf_primitive);
	auto load_mesh_primitive_indices(tinygltf::Primitive const &gltf_primitive) -> u32;

	auto get_primitive_attribute_buffer(
	    const tinygltf::Primitive &gltf_primitive,
	    const char *attribute_name
	) -> const f32 *;

	auto get_primitive_vertex_count(tinygltf::Primitive const &gltf_primitive) -> usize;
	auto get_primitive_index_count(tinygltf::Primitive const &gltf_primitive) -> usize;

	void set_initial_node_transform(tinygltf::Node const &gltf_node, Model::Node *node);

	auto node_has_any_children(tinygltf::Node const &gltf_node) -> bool;
	auto node_has_any_mesh(tinygltf::Node const &gltf_node) -> bool;

private:
	VkContext const *const vk_context = {};
	TextureLoader const *const texture_loader = {};

	tinygltf::Model gltf_model = {};
	Model model = {};

	Buffer *staging_vertex_buffer = {};
	Buffer *staging_index_buffer = {};
	Buffer *staging_texture_buffer = {};

	Model::Vertex *vertex_map = {};
	u32 *index_map = {};

	usize vertex_count = {};
	usize index_count = {};
};

} // namespace BINDLESSVK_NAMESPACE
