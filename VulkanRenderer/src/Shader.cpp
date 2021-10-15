#include "Shader.h"

Shader::Shader(const std::string& vertexPath, const std::string& pixelPath, SharedContext sharedContext) :
	m_SharedContext(sharedContext)
{
	// compile glsl to spv
	shaderc::SpvCompilationResult vertexResult = CompileGlslToSpv(vertexPath, Stage::VERTEX);
	shaderc::SpvCompilationResult pixelResult = CompileGlslToSpv(pixelPath, Stage::PIXEL);

	if (pixelResult.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LOG(err, "Shader::Shader: failed to compile vertex shader at {}", vertexPath);
		LOG(err, "{}", vertexResult.GetErrorMessage());
	}

	if (pixelResult.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LOG(err, "Shader::Shader: failed to compile vertex shader at {}", pixelPath);
		LOG(err, "{}", pixelResult.GetErrorMessage());
	}

	std::vector<uint32_t> vertexSpirv = { vertexResult.cbegin(), vertexResult.cend() };
	std::vector<uint32_t> pixelSpirv = { pixelResult.cbegin(), pixelResult.cend() };

	// vertex-shader module create-info
	VkShaderModuleCreateInfo vertexShaderModuleCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vertexSpirv.size() * sizeof(uint32_t),
		.pCode = vertexSpirv.data()
	};

	// pixel-shader module create-info
	VkShaderModuleCreateInfo pixelShaderModuleCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = pixelSpirv.size() * sizeof(uint32_t),
		.pCode = pixelSpirv.data()
	};

	// create shader modules
	VKC(vkCreateShaderModule(m_SharedContext.device, &vertexShaderModuleCreateInfo, nullptr, &m_PixelShaderModule));
	VKC(vkCreateShaderModule(m_SharedContext.device, &pixelShaderModuleCreateInfo, nullptr, &m_VertexShaderModule));

	// pipeline vertex-shader stage create-info
	VkPipelineShaderStageCreateInfo pipelineVertexShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
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
	vkDestroyShaderModule(m_SharedContext.device, m_VertexShaderModule, nullptr);
	vkDestroyShaderModule(m_SharedContext.device, m_PixelShaderModule, nullptr);
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
	return compiler.CompileGlslToSpv(sourceText, stage == Stage::VERTEX ? shaderc_shader_kind::shaderc_vertex_shader : shaderc_shader_kind::shaderc_fragment_shader, fileName.c_str(), options);
}