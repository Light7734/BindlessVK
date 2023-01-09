#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Model.hpp"
#include "BindlessVk/Texture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// forward declarations
namespace tinygltf {
struct Node;
struct Model;
struct Mesh;
struct Primitive;
} // namespace tinygltf

namespace BINDLESSVK_NAMESPACE {

/**
 * @todo refactor storing model textures?
 */
class ModelLoader
{
private:
	struct MeshData
	{
		std::vector<Model::Vertex> vertex_buffer;
		std::vector<uint32_t> index_buffer;
	};

public:
	ModelLoader() = default;

	/** Initializes the model system
	 * @param device the bindlessvk device
	 * @param texture_system the bindlessvk texture system, ModelLoader may load textures
	 */
	void init(Device* device, TextureSystem* texture_system);

	/** Loads a model from a gltf file
	 * @param name debug name attached to vulkan objects for debugging tools like renderdoc
	 * @param gltf_path path to the gltf model file
	 */
	Model load_from_gltf(const char* name, const char* gltf_path);

private:
	tinygltf::Model load_gltf_file(const char* gltf_path);
	MeshData load_mesh(const tinygltf::Model& gltf_model, Model& model);
	void load_textures(const tinygltf::Model& gltf_model, Model& model);
	void load_material_parameters(const tinygltf::Model& gltf_model, Model& model);
	void copy_mesh_data_to_gpu(Model& model, MeshData& mesh_data);

	Model::Node* load_node(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Node& gltf_node,
	    Model& model,
	    Model::Node* parent_node,
	    MeshData& out_mesh_data
	);

	void load_mesh_primitives(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Mesh& gltf_mesh,
	    Model::Node* node,
	    MeshData& out_mesh_data
	);

	void load_mesh_primitive_vertices(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive,
	    MeshData& out_mesh_data
	);

	u32 load_mesh_primitive_indices(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive,
	    MeshData& out_mesh_data
	);

	const float* get_mesh_primitive_position_buffer(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	const float* get_mesh_primitive_normal_buffer(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	const float* get_mesh_primitive_tangent_buffer(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	const float* get_mesh_primitive_uv_buffer(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	size_t get_mesh_primitive_vertex_count(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	u32 get_mesh_primitive_index_count(
	    const tinygltf::Model& gltf_model,
	    const tinygltf::Primitive& gltf_primitive
	);

	void set_initial_node_transform(const tinygltf::Node& gltf_node, Model::Node* node);

	bool node_has_any_children(const tinygltf::Node& gltf_node);
	bool node_has_any_mesh(const tinygltf::Node& gltf_node);

private:
	Device* device;
	TextureSystem* texture_system;

	vk::CommandPool command_pool;
};

} // namespace BINDLESSVK_NAMESPACE
