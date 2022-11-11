#include "Graphics/Model.hpp"

#include "tiny_gltf.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

ModelSystem::ModelSystem(const ModelSystem::CreateInfo& info)
    : m_LogicalDevice(info.deviceContext.logicalDevice)
    , m_PhysicalDevice(info.deviceContext.physicalDevice)
    , m_Allocator(info.deviceContext.allocator)
    , m_CommandPool(info.commandPool)
    , m_GraphicsQueue(info.graphicsQueue)
{
}

void ModelSystem::LoadModel(const Model::CreateInfo& info)
{
	tinygltf::Model input;
	tinygltf::TinyGLTF gltfContext;
	std::string err, warn;

	std::vector<uint32_t> indices;
	std::vector<Model::Vertex> vertices;

	Model& model = m_Models[HashStr(info.name)];


	ASSERT(gltfContext.LoadASCIIFromFile(&input, &err, &warn, info.gltfPath), "Failed to load gltf file: {}@{}\nerr: {}", info.name, info.gltfPath, err);
	if (!warn.empty())
	{
		LOG(warn, warn);
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Load textures
	{
		for (tinygltf::Image& image : input.images)
		{
			model.textures.push_back(
			    info.textureSystem.CreateFromGLTF({
			        .image = &image,
			    }));
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Load material parameters
	{
		for (tinygltf::Material& material : input.materials)
		{
			ASSERT(material.values.find("baseColorTexture") != material.values.end(),
			       "Material doesn't have required values");

			model.materialParameters.push_back({
			    .albedoFactor       = glm::vec4(1.0),
			    .albedoTextureIndex = material.values["baseColorTexture"].TextureIndex(),
			});
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	/// Load nodes
	{
		std::function<void(const tinygltf::Node&, Model::Node*)> loadNode =
		    [&](const tinygltf::Node& inputNode, Model::Node* parent) {
			    Model::Node* node = new Model::Node {};

			    if (parent)
			    {
				    parent->children.push_back(node);
			    }
			    else
			    {
				    model.nodes.push_back(node);
			    }

			    node->parent = parent;

			    if (inputNode.translation.size() == 3)
			    {
				    node->transform = glm::translate(node->transform, glm::vec3(glm::make_vec3(inputNode.translation.data())));
			    }
			    if (inputNode.rotation.size() == 4)
			    {
				    glm::quat q = glm::make_quat(inputNode.rotation.data());
				    node->transform *= glm::mat4(q);
			    }
			    if (inputNode.scale.size() == 3)
			    {
				    node->transform = glm::scale(node->transform, glm::vec3(glm::make_vec3(inputNode.scale.data())));
			    }
			    if (inputNode.matrix.size() == 16)
			    {
				    node->transform = glm::make_mat4x4(inputNode.matrix.data());
			    };

			    if (inputNode.children.size() > 0)
			    {
				    for (int32_t childIndex : inputNode.children)
				    {
					    loadNode(input.nodes[childIndex], node);
				    }
			    }

			    if (inputNode.mesh > -1)
			    {
				    tinygltf::Mesh mesh = input.meshes[inputNode.mesh];

				    for (const tinygltf::Primitive& primitive : mesh.primitives)
				    {
					    uint32_t firstIndex  = static_cast<uint32_t>(indices.size());
					    uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
					    uint32_t indexCount  = 0ul;


					    // Vertices
					    {
						    const float* positionBuffer = nullptr;
						    const float* normalBuffer   = nullptr;
						    const float* tangentBuffer  = nullptr;
						    const float* texCoordBuffer = nullptr;
						    size_t vertexCount          = 0ul;

						    if (primitive.attributes.find("POSITION") != primitive.attributes.end())
						    {
							    const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("POSITION")->second];
							    const tinygltf::BufferView& view   = input.bufferViews[accessor.bufferView];

							    positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
							    vertexCount    = accessor.count;
						    }

						    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
						    {
							    const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("NORMAL")->second];
							    const tinygltf::BufferView& view   = input.bufferViews[accessor.bufferView];
							    normalBuffer                       = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						    }

						    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
						    {
							    const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("TANGENT")->second];
							    const tinygltf::BufferView& view   = input.bufferViews[accessor.bufferView];
							    tangentBuffer                      = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						    }
						    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
						    {
							    const tinygltf::Accessor& accessor = input.accessors[primitive.attributes.find("TEXCOORD_0")->second];
							    const tinygltf::BufferView& view   = input.bufferViews[accessor.bufferView];
							    texCoordBuffer                     = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						    }

						    for (size_t v = 0; v < vertexCount; ++v)
						    {
							    vertices.push_back({
							        .position = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f),
							        .normal   = glm::normalize(glm::vec3(normalBuffer ? glm::make_vec3(&normalBuffer[v * 3]) : glm::vec3(0.0f))),
							        .tangent  = glm::normalize(glm::vec3(tangentBuffer ? glm::make_vec3(&tangentBuffer[v * 3]) : glm::vec3(0.0f))),
							        .uv       = texCoordBuffer ? glm::make_vec2(&texCoordBuffer[v * 2]) : glm::vec3(0.0f),
							        .color    = glm::vec3(1.0),
							    });
						    }
					    }

					    // Indices
					    {
						    const tinygltf::Accessor& accessor     = input.accessors[primitive.indices];
						    const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
						    const tinygltf::Buffer& buffer         = input.buffers[bufferView.buffer];

						    indexCount += static_cast<uint32_t>(accessor.count);

						    switch (accessor.componentType)
						    {
						    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
						    {
							    const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							    for (size_t index = 0; index < accessor.count; index++)
							    {
								    indices.push_back(buf[index] + vertexStart);
							    }
							    break;
						    }
						    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						    {
							    const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							    for (size_t index = 0; index < accessor.count; index++)
							    {
								    indices.push_back(buf[index] + vertexStart);
							    }
							    break;
						    }
						    case TINYGLTF_PARAMETER_TYPE_BYTE:
						    {
							    const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							    for (size_t index = 0; index < accessor.count; index++)
							    {
								    indices.push_back(buf[index] + vertexStart);
							    }
							    break;
						    }
						    }
					    }

					    node->mesh.push_back({
					        .firstIndex    = firstIndex,
					        .indexCount    = indexCount,
					        .materialIndex = primitive.material,
					    });
				    }
			    }
		    };

		for (size_t nodeIndex : input.scenes[0].nodes)
		{
			loadNode(input.nodes[nodeIndex], nullptr);
		}
	} // Load nodes

	model.vertexBuffer = new StagingBuffer({
	    .logicalDevice  = m_LogicalDevice,
	    .physicalDevice = m_PhysicalDevice,
	    .allocator      = m_Allocator,
	    .commandPool    = m_CommandPool,
	    .graphicsQueue  = m_GraphicsQueue,
	    .usage          = vk::BufferUsageFlagBits::eVertexBuffer,
	    .minBlockSize   = vertices.size() * sizeof(Model::Vertex),
        .blockCount = 1u,
	    .initialData    = vertices.data(),
	});

	std::string vertexBufferName(std::string(info.name) + " VertexBuffer");
	m_LogicalDevice.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (uint64_t)(VkBuffer)(*model.vertexBuffer->GetBuffer()),
	    vertexBufferName.c_str(),
	});


	model.indexBuffer = new StagingBuffer({
	    .logicalDevice  = m_LogicalDevice,
	    .physicalDevice = m_PhysicalDevice,
	    .allocator      = m_Allocator,
	    .commandPool    = m_CommandPool,
	    .graphicsQueue  = m_GraphicsQueue,
	    .usage          = vk::BufferUsageFlagBits::eIndexBuffer,
	    .minBlockSize   = indices.size() * sizeof(uint32_t),
	    .blockCount     = 1u,
	    .initialData    = indices.data(),
	});

	std::string indexBufferName(std::string(info.name) + " IndexBuffer");
	m_LogicalDevice.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (uint64_t)(VkBuffer)(*model.indexBuffer->GetBuffer()),
	    indexBufferName.c_str(),
	});
}

ModelSystem::~ModelSystem()
{
}
