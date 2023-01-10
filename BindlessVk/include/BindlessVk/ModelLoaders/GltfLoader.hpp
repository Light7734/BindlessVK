#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Model.hpp"
#include "BindlessVk/Texture.hpp"

#include <glm/glm.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

/**
 * @todo Implement load_from_binary
 * @todo Implement parse_binary_file
 */
class GltfLoader
{
private:
	struct MeshData
	{
		std::vector<Model::Vertex> vertex_buffer;
		std::vector<uint32_t> index_buffer;
	};

public:
	GltfLoader(Device* device, TextureSystem* texture_system);
	GltfLoader() = default;
	~GltfLoader() = default;

	Model load_from_ascii(const char* debug_name, const char* file_path);
	Model load_from_binary(const char* debug_name, const char* file_path) = delete;

private:
	void parse_ascii_file(const char* debug_name, const char* file_path);
	void parse_binary_file(const char* debug_name, const char* file_path) = delete;

	void load_textures();
	void load_material_parameters();
	void load_mesh();
	void copy_mesh_data_to_gpu();

	Model::Node* load_node(const tinygltf::Node& gltf_node, Model::Node* parent_node);

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
	TextureSystem* texture_system = {};

	tinygltf::Model gltf_model;
	Model model;

	MeshData mesh_data;
};

} // namespace BINDLESSVK_NAMESPACE
