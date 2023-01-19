#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Model.hpp"
#include "BindlessVk/TextureLoader.hpp"

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
	    Device* device,
	    TextureLoader* texture_loader,
	    Buffer* staging_vertex_buffer,
	    Buffer* staging_index_buffer
	);
	GltfLoader() = default;
	~GltfLoader() = default;

	Model load_from_ascii(const char* debug_name, const char* file_path);
	Model load_from_binary(const char* debug_name, const char* file_path) = delete;

private:
	void load_gltf_model_from_ascii(const char* file_path);
	void load_gltf_model_from_binary(const char* file_path) = delete;

	void load_textures();
	void load_material_parameters();
	void load_mesh_data_to_staging_buffers();

	void write_mesh_data_to_gpu();

	Model::Node* load_node(const tinygltf::Node& gltf_node, Model::Node* parent_node);

	void write_vertex_buffer_to_gpu();
	void write_index_buffer_to_gpu();

	void load_mesh_primitives(const tinygltf::Mesh& gltf_mesh, Model::Node* node);

	void load_mesh_primitive_vertices(const tinygltf::Primitive& gltf_primitive);
	u32 load_mesh_primitive_indices(const tinygltf::Primitive& gltf_primitive);

	const f32* get_primitive_attribute_buffer(
	    const tinygltf::Primitive& gltf_primitive,
	    const char* attribute_name
	);

	usize get_primitive_vertex_count(const tinygltf::Primitive& gltf_primitive);
	usize get_primitive_index_count(const tinygltf::Primitive& gltf_primitive);

	void set_initial_node_transform(const tinygltf::Node& gltf_node, Model::Node* node);

	bool node_has_any_children(const tinygltf::Node& gltf_node);
	bool node_has_any_mesh(const tinygltf::Node& gltf_node);

private:
	Device* device = {};
	TextureLoader* texture_loader = {};

	tinygltf::Model gltf_model = {};
	Model model = {};

	Buffer* staging_vertex_buffer = {};
	Buffer* staging_index_buffer = {};

	Model::Vertex* vertex_map = {};
	u32* index_map = {};

	usize vertex_count = {};
	usize index_count = {};
};

} // namespace BINDLESSVK_NAMESPACE
