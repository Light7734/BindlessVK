// #pragma once
//
// #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
// #include "Core/Base.hpp"
//
// #include <shaderc/shaderc.hpp>
// #include <vulkan/vulkan.hpp>
//
// struct ShaderCreateInfo
// {
// 	vk::Device logicalDevice;
// 	shaderc_optimization_level optimizationLevel;
// 	const std::string vertexPath;
// 	const std::string pixelPath;
// };
//
// class Shader
// {
// public:
// 	Shader(ShaderCreateInfo& createInfo);
// 	~Shader();
//
// 	inline uint32_t GetStageCount() const { return 2u; }; // TODO: Geometry shader
// 	inline vk::PipelineShaderStageCreateInfo* GetShaderStageCreateInfos() { return &m_PipelineShaderCreateInfos[0]; }
//
// private:
// 	// Shader stages enum
// 	enum class Stage : uint8_t
// 	{
// 		None = 0,
// 		Vertex,
// 		Pixel,
// 		Geometry,
// 	};
//
// 	// Device
// 	vk::Device m_LogicalDevice = {};
//
// 	// Modules
// 	vk::ShaderModule m_VertexShaderModule = {};
// 	vk::ShaderModule m_PixelShaderModule  = {};
//
// 	// Pipeline
// 	vk::PipelineShaderStageCreateInfo m_PipelineShaderCreateInfos[2] = {};
//
// 	shaderc::SpvCompilationResult CompileGlslToSpv(const std::string& path, Stage stage, shaderc_optimization_level optimizationLevel);
// };
