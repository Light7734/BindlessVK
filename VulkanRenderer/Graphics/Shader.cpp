// #include "Graphics/Shader.hpp"
//
// #include <filesystem>
// #include <fstream>
//
// Shader::Shader(ShaderCreateInfo& createInfo)
//     : m_LogicalDevice(createInfo.logicalDevice)
// {
// 	/////////////////////////////////////////////////////////////////////////////////
// 	// Compile vertex shader and create shader module create-info
// 	auto vertexSpv = CompileGlslToSpv(createInfo.vertexPath, Stage::Vertex, createInfo.optimizationLevel);
//
// 	vk::ShaderModuleCreateInfo vertexCreateInfo {
// 		{},                                                              // flags
// 		static_cast<size_t>(vertexSpv.end() - vertexSpv.begin()) * 4ull, // codeSize
// 		vertexSpv.begin(),                                               // pCode
// 	};
// 	m_VertexShaderModule = m_LogicalDevice.createShaderModule(vertexCreateInfo, nullptr);
//
// 	m_PipelineShaderCreateInfos[0] = {
// 		{},                               // flags
// 		vk::ShaderStageFlagBits::eVertex, // stage
// 		m_VertexShaderModule,             // module
// 		"main",                           // pName
// 	};
//
// 	/////////////////////////////////////////////////////////////////////////////////
// 	// Compile pixel shader and create shader module create-info
// 	auto pixelSpv = CompileGlslToSpv(createInfo.pixelPath, Stage::Pixel, createInfo.optimizationLevel);
//
// 	vk::ShaderModuleCreateInfo pixelCreateInfo {
// 		{},                                                            // flags
// 		static_cast<size_t>(pixelSpv.end() - pixelSpv.begin()) * 4ull, // codeSize
// 		pixelSpv.begin(),                                              // pCode
// 	};
// 	m_PixelShaderModule = m_LogicalDevice.createShaderModule(pixelCreateInfo, nullptr);
//
// 	m_PipelineShaderCreateInfos[1] = {
// 		{},                                 // flags
// 		vk::ShaderStageFlagBits::eFragment, // stage
// 		m_PixelShaderModule,                // module
// 		"main",                             // pName
// 	};
// }
//
// Shader::~Shader()
// {
// 	m_LogicalDevice.destroyShaderModule(m_VertexShaderModule, nullptr);
// 	m_LogicalDevice.destroyShaderModule(m_PixelShaderModule, nullptr);
// }
//
// shaderc::SpvCompilationResult Shader::CompileGlslToSpv(const std::string& path, Stage stage, shaderc_optimization_level optimizationLevel)
// {
// 	// compile options
// 	shaderc::CompileOptions options;
// 	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
// 	options.SetOptimizationLevel(optimizationLevel);
//
// 	// read shader file
// 	std::ifstream file(path, std::ios::ate | std::ios::binary | std::ios::in);
// 	std::string sourceText = "";
// 	std::string fileName   = path.substr(path.find_last_of("/") + 1);
// 	LOG(warn, "Working dir: {}", std::filesystem::current_path().c_str());
// 	if (file)
// 	{
// 		sourceText.resize(static_cast<size_t>(file.tellg()));
// 		file.seekg(0, std::ios::beg);
// 		file.read(&sourceText[0], sourceText.size());
// 		sourceText.resize(static_cast<size_t>(file.gcount()));
// 		file.close();
// 	}
//
// 	// compile
// 	shaderc::Compiler compiler;
// 	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(sourceText, stage == Stage::Vertex ? shaderc_shader_kind::shaderc_vertex_shader : shaderc_shader_kind::shaderc_fragment_shader, fileName.c_str(), options);
//
// 	// log error
// 	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
// 	{
// 		LOG(err, "Failed to compile {} shader at {}...", stage == Stage::Vertex ? "vertex" : "pixel", path);
// 		LOG(err, "    {}", result.GetErrorMessage());
// 	}
//
// 	return result;
// }
