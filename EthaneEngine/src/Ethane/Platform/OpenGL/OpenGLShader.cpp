#include "ethpch.h"
#include "OpenGLShader.h"

#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "Ethane/Core/Timer.h"

namespace Ethane {

	namespace Utils {

		static GLenum ShaderTypeFromString(const std::string& type)
		{
			if (type == "vertex")
				return GL_VERTEX_SHADER;
			if (type == "fragment" || type == "pixel")
				return GL_FRAGMENT_SHADER;

			ETH_CORE_ASSERT(false, "Unknown shader type!");
			return 0;
		}

		static shaderc_shader_kind GLShaderStageToShaderC(GLenum stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:   return shaderc_glsl_vertex_shader;
			case GL_FRAGMENT_SHADER: return shaderc_glsl_fragment_shader;
			}
			ETH_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}

		static const char* GLShaderStageToString(GLenum stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:   return "GL_VERTEX_SHADER";
			case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
			}
			ETH_CORE_ASSERT(false);
			return nullptr;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "assets/cache/shader/opengl";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static const char* GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
			case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
			}
			ETH_CORE_ASSERT(false);
			return "";
		}

		static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
		{
			switch (stage)
			{
			case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
			case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
			}
			ETH_CORE_ASSERT(false);
			return "";
		}
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
		: m_FilePath(filepath)
	{
		ETH_PROFILE_FUNCTION();

		Utils::CreateCacheDirectoryIfNeeded();

		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);
		
		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSources);
			CompileOrGetOpenGLBinaries();
			CreateProgram();
			ETH_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
		}

		//Exrtact name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
		:m_Name(name)
	{
		ETH_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		
		{
			Timer timer;
			CompileOrGetVulkanBinaries(sources);
			CompileOrGetOpenGLBinaries();
			CreateProgram();
			ETH_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
		}
	}

	OpenGLShader::~OpenGLShader()
	{
		ETH_PROFILE_FUNCTION();

		glDeleteProgram(m_RendererID);
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
		ETH_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			ETH_CORE_ASSERT(eol != std::string::npos, "Syntex error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			ETH_CORE_ASSERT(Utils::ShaderTypeFromString(type), "Invalid shader type specification!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			ETH_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
			pos = source.find(typeToken, nextLinePos);
			shaderSources[Utils::ShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		return shaderSources;
	}

	void OpenGLShader::CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		ETH_PROFILE_FUNCTION();

		// test
		// GLuint program = glCreateProgram();
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = false;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();
		for (auto&& [stage, source] : shaderSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && m_UseCache) // test fix tomorrow
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);
				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ETH_CORE_ERROR(module.GetErrorMessage());
					ETH_CORE_ASSERT(false);
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

	}

	void OpenGLShader::CompileOrGetOpenGLBinaries()
	{
		auto& shaderData = m_OpenGLSPIRV;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_opengl_compat, shaderc_env_version_opengl_4_5);
		const bool optimize = false;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		shaderData.clear();
		m_OpenGLSourceCode.clear();
		for (auto&& [stage, spirv] : m_VulkanSPIRV)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension(stage));
			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && m_UseCache)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				spirv_cross::CompilerGLSL glslCompiler(spirv);
				m_OpenGLSourceCode[stage] = glslCompiler.compile();
				auto& source = m_OpenGLSourceCode[stage];

				// test
				printf("=========================================\n");
				printf("%s Shader:\n%s\n", Utils::GLShaderStageToString(stage), source.c_str());
				printf("=========================================\n");
				// testend

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ETH_CORE_ERROR(module.GetErrorMessage());
					ETH_CORE_ASSERT(false);
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		for (auto&& [stage, data] : shaderData)
			Reflect(stage, data);
	}

	void OpenGLShader::CreateProgram()
	{
		ETH_PROFILE_FUNCTION();

		GLuint program = glCreateProgram();

		std::vector<GLuint> shaderIDs;
		for (auto&& [stage, spirv] : m_OpenGLSPIRV)
		{
			GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
			glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
			glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
			glAttachShader(program, shaderID);
		}

		glLinkProgram(program);
		GLint isLinked;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);
			// Don't leak shaders either.
			for (auto id : shaderIDs)
				glDeleteShader(id);

			// Use the infoLog as you see fit.
			ETH_CORE_ERROR("{0}", infoLog.data());
			ETH_CORE_ASSERT(false, "Shader link failure!");
			// In this simple program, we'll just leave
			return;
		}

		// Always detach shaders after a successful link.
		for (auto id : shaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}

		m_RendererID = program;


		// test
		// ETH_CORE_INFO("uniform binding");
		// uint32_t uboIndex = glGetUniformBlockIndex(program, "Transform");
		// glUniformBlockBinding(program, 1, uboIndex);
		// uboIndex = glGetUniformBlockIndex(program, "Camera");
		// glUniformBlockBinding(program, 0, uboIndex);
	}

	void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t>& shaderData)
	{
		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources res = compiler.get_shader_resources();

		ETH_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", Utils::GLShaderStageToString(stage), m_FilePath);
		ETH_CORE_TRACE("    {0} uniform buffers", res.uniform_buffers.size());
		ETH_CORE_TRACE("    {0} resources", res.sampled_images.size());

		ETH_CORE_TRACE("Uniform buffers:");
		glUseProgram(m_RendererID);
		for (const auto& resource : res.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = bufferType.member_types.size();

			ETH_CORE_TRACE("    {0}, {1}", resource.name, compiler.get_name(resource.id));
			ETH_CORE_TRACE("    Size = {0}", bufferSize);
			ETH_CORE_TRACE("    Binding = {0}", binding);
			ETH_CORE_TRACE("    Members = {0}", memberCount);

			if (s_UniformBuffers.find(binding) == s_UniformBuffers.end())
			{
				UniformBufferSpec& buffer = s_UniformBuffers[binding];
				buffer.Name = resource.name;
				buffer.BindingPoint = binding;
				buffer.Size = bufferSize;

				buffer.Uniforms.reserve(memberCount);
				for (int i = 0; i < memberCount; i++)
				{
					auto type = compiler.get_type(bufferType.member_types[i]);
					const auto& name = compiler.get_member_name(bufferType.self, i);
					auto size = compiler.get_declared_struct_member_size(bufferType, i);
					auto offset = compiler.type_struct_member_offset(bufferType, i);
					ETH_CORE_INFO("member name: {0}", name);
					ETH_CORE_INFO("member size: {0}", size);
					ETH_CORE_INFO("member offset: {0}", offset);

					//ShaderUniformType uniformType = SPIRTypeToShaderUniformType(type);
					//buffer.Uniforms.emplace_back(name, uniformType, size, offset);
				}

				glCreateBuffers(1, &buffer.RendererID);
				glNamedBufferData(buffer.RendererID, buffer.Size, NULL, GL_DYNAMIC_DRAW); 
				glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);

				ETH_CORE_TRACE("Created Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
				// glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}
			else
			{
				// Validation
				UniformBufferSpec& buffer = s_UniformBuffers.at(binding);
				// ETH_CORE_INFO("{0}, {1}", buffer.Name, resource.name);
				ETH_CORE_ASSERT(buffer.Name == resource.name); // Must be the same buffer
				if (bufferSize > buffer.Size) // Resize buffer if needed
				{
					buffer.Size = bufferSize;

					glDeleteBuffers(1, &buffer.RendererID);
					glCreateBuffers(1, &buffer.RendererID);
					glNamedBufferData(buffer.RendererID, buffer.Size, NULL, GL_DYNAMIC_DRAW);
					glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);

					ETH_CORE_TRACE("Resized Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
				}
			}
		}
		// test
		// for (const spirv_cross::Resource& resource : res.push_constant_buffers)
		// {
		// 	const auto& bufferName = resource.name;
		// 	auto& bufferType = compiler.get_type(resource.base_type_id);
		// 	const auto bufferSize = uint32_t(compiler.get_declared_struct_size(bufferType));
		// 
		// 	// Skip empty push constant buffers - these are for the renderer only
		// 	if (bufferName.empty() || bufferName == "u_Renderer")
		// 	{
		// 		m_ConstantBufferOffset += bufferSize;
		// 		continue;
		// 	}
		// 
		// 	auto location = compiler.get_decoration(resource.id, spv::DecorationLocation);
		// 	const int memberCount = int(bufferType.member_types.size());
		// 	auto& [Name, Size, Uniforms] = m_Buffers[bufferName];
		// 	Name = bufferName;
		// 	Size = bufferSize - m_ConstantBufferOffset;
		// 	for (int i = 0; i < memberCount; i++)
		// 	{
		// 		const auto& type = compiler.get_type(bufferType.member_types[i]);
		// 		const auto& memberName = compiler.get_member_name(bufferType.self, i);
		// 		const auto size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, i);
		// 		const auto offset = compiler.type_struct_member_offset(bufferType, i) - m_ConstantBufferOffset;
		// 
		// 		std::string uniformName = fmt::format("{}.{}", bufferName, memberName);
		// 		Uniforms[uniformName] = ShaderUniform(uniformName, SPIRTypeToShaderUniformType(type), size, offset);
		// 	}
		// 
		// 	m_ConstantBufferOffset += bufferSize;
		// }
		// testend
	}

	void OpenGLShader::Bind() const
	{
		ETH_PROFILE_FUNCTION();

		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		ETH_PROFILE_FUNCTION();

		glUseProgram(0);
	}

	// Uniforms
	void OpenGLShader::SetUniform(const std::string& name, const int value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformInt(name, value);
	}

	void OpenGLShader::SetUniform(const std::string& name, int* values, uint32_t count)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformIntArray(name, values, count);
	}

	void OpenGLShader::SetUniform(const std::string& name, const float value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetUniform(const std::string& name, const glm::vec2& value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformFloat2(name, value);
	}
	void OpenGLShader::SetUniform(const std::string& name, const glm::ivec2& value)
	{
		ETH_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2i(location, value.x, value.y);
	}

	void OpenGLShader::SetUniform(const std::string& name, const glm::vec3& value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformFloat3(name, value);
	}
	void OpenGLShader::SetUniform(const std::string& name, const glm::ivec3& value)
	{
		ETH_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3i(location, value.x, value.y, value.z);
	}

	void OpenGLShader::SetUniform(const std::string& name, const glm::vec4& value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformFloat4(name, value);
	}
	void OpenGLShader::SetUniform(const std::string& name, const glm::ivec4& value)
	{
		ETH_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4i(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::SetUniform(const std::string& name, const glm::mat3& value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformMat3(name, value);
	}

	void OpenGLShader::SetUniform(const std::string& name, const glm::mat4& value)
	{
		ETH_PROFILE_FUNCTION();

		UploadUniformMat4(name, value);
	}

	// internal Uniform
	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}


	// UniformBuffer
	uint32_t OpenGLShader::GetUniformBufferIndex(uint32_t bindingPoint)
	{
		ETH_CORE_ASSERT(s_UniformBuffers.size() > bindingPoint, "Can't find uniform buffer");
		return s_UniformBuffers[bindingPoint].RendererID;
		ETH_CORE_ASSERT(false, "Can't find uniform buffer");
	}

	uint32_t OpenGLShader::GetUniformBufferIndex(const std::string& name)
	{
		for (auto& [bindingPoint, ub] : s_UniformBuffers)
		{
			if (ub.Name == name)
			{
				return ub.RendererID;
			}
		}
		ETH_CORE_ASSERT(false, "Can't find uniform buffer");
	}

	void OpenGLShader::SetUniformBuffer(uint32_t uboIndex, const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(uboIndex, offset, size, data);
	}

	void OpenGLShader::SetUniformBufferByBindingPoint(uint32_t bindingPoint, const void* data, uint32_t size, uint32_t offset)
	{
		uint32_t uboIndex = GetUniformBufferIndex(bindingPoint);
		glNamedBufferSubData(uboIndex, 0, size, data);
	}

	void OpenGLShader::SetUniformBufferByName(const std::string& name, const void* data, uint32_t size)
	{
		uint32_t uboIndex = GetUniformBufferIndex(name);
		glNamedBufferSubData(uboIndex, 0, size, data);
	}
}