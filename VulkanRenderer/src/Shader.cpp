#include "Shader.h"


Shader::Shader(const std::string& vertexPath, const std::string& pixelPath, SharedContext sharedContext) :
	m_SharedContext(sharedContext)
{
	// compile glsl to spv
	shaderc::SpvCompilationResult vertexResult = CompileGlslToSpv(vertexPath, Stage::VERTEX);
	shaderc::SpvCompilationResult pixelResult = CompileGlslToSpv(pixelPath, Stage::PIXEL);

	// vertex-shader module create-info
	VkShaderModuleCreateInfo vertexShaderModuleCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = static_cast<size_t>(vertexResult.end() - vertexResult.begin()) * 4ull,
		.pCode = vertexResult.begin()
	};

	// pixel-shader module create-info
	VkShaderModuleCreateInfo pixelShaderModuleCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = static_cast<size_t>(pixelResult.end() - pixelResult.begin()) * 4ull,
		.pCode = pixelResult.begin()
	};

	// create shader modules
	VKC(vkCreateShaderModule(m_SharedContext.logicalDevice, &vertexShaderModuleCreateInfo, nullptr, &m_VertexShaderModule));
	VKC(vkCreateShaderModule(m_SharedContext.logicalDevice, &pixelShaderModuleCreateInfo, nullptr, &m_PixelShaderModule));

	// pipeline vertex-shader stage create-info
	VkPipelineShaderStageCreateInfo pipelineVertexShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = m_VertexShaderModule,
		.pName = "main"
	};

	// pipeline pixel-shader stage create-info
	VkPipelineShaderStageCreateInfo pipelinePixelShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = m_PixelShaderModule,
		.pName = "main"
	};

	// store pipeline shader stage create-infos
	m_PipelineShaderStageCreateInfos[0] = pipelineVertexShaderStageCreateInfo;
	m_PipelineShaderStageCreateInfos[1] = pipelinePixelShaderStageCreateInfo;
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_SharedContext.logicalDevice, m_VertexShaderModule, nullptr);
	vkDestroyShaderModule(m_SharedContext.logicalDevice, m_PixelShaderModule, nullptr);
}

shaderc::SpvCompilationResult Shader::CompileGlslToSpv(const std::string& path, Stage stage)
{
	// compile options
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	// read shader file
	std::ifstream file;
	std::string sourceText = "";
	std::string fileName = path.substr(path.find_last_of("/") + 1);

	file.open(path, std::ios_base::in | std::ios_base::binary);
	ASSERT(file, "Shader::CompileGlslToSpv: failed to open {} shader file at: {}", stage == Stage::VERTEX ? "vertex" : "pixel", path);
	if (file)
	{
		file.seekg(0, std::ios::end);
		sourceText.resize(static_cast<size_t>(file.tellg()));
		file.seekg(0, std::ios::beg);
		file.read(&sourceText[0], sourceText.size());
		sourceText.resize(static_cast<size_t>(file.gcount()));
		file.close();
	}

	// compile
	shaderc::Compiler compiler;
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(sourceText, stage == Stage::VERTEX ? shaderc_shader_kind::shaderc_vertex_shader : shaderc_shader_kind::shaderc_fragment_shader, fileName.c_str(), options);

	// log err
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LOG(err, "Shader::CompileGlslToSpv: failed to compile {} shader at {}...", stage == Stage::VERTEX ? "vertex" : "pixel", path);
		LOG(err, "Shader::CompileGlslToSpv: {}", result.GetErrorMessage());

		return result;
	}

	return result;
}