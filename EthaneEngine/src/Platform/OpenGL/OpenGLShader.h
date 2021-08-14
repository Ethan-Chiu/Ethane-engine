#pragma once

#include "Ethane/Renderer/Shader.h"
#include <glm/glm.hpp>


#include "Ethane/Renderer/UniformBuffer.h"

//TODO: remove
typedef unsigned int GLenum;

namespace Ethane {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		// Uniform Varible
		virtual void SetUniform(const std::string& name, const float value); // float
		virtual void SetUniform(const std::string& name, const int value); // int 
		virtual void SetUniform(const std::string& name, int* values, uint32_t count); // intArray
		virtual void SetUniform(const std::string& name, const glm::vec2& value); // vec2
		virtual void SetUniform(const std::string& name, const glm::ivec2& value); // int vec2
		virtual void SetUniform(const std::string& name, const glm::vec3& value); // vec3
		virtual void SetUniform(const std::string& name, const glm::ivec3& value); // int vec3
		virtual void SetUniform(const std::string& name, const glm::vec4& value); // vec4
		virtual void SetUniform(const std::string& name, const glm::ivec4& value); // int vec4
		virtual void SetUniform(const std::string& name, const glm::mat3& value); // mat3
		virtual void SetUniform(const std::string& name, const glm::mat4& value); // mat4

		virtual const std::string& GetName() const override { return m_Name; }

		// Uniform Varible internal
		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);


		// UniformBuffer //prehaps move to opengl uniformbuffer
		uint32_t GetUniformBufferIndex(uint32_t bindingPoint) override;
		uint32_t GetUniformBufferIndex(const std::string& name) override;
		void SetUniformBuffer(uint32_t uboIndex, const void* data, uint32_t size, uint32_t offset = 0) override;
		void SetUniformBufferByBindingPoint(uint32_t bindingPoint, const void* data, uint32_t size, uint32_t offset = 0) override;
		void SetUniformBufferByName(const std::string& name, const void* data, uint32_t size) override;

	private:
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

		void CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources);
		void CompileOrGetOpenGLBinaries();
		void CreateProgram();
		void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);
	private:
		uint32_t m_RendererID;
		std::string m_FilePath;
		std::string m_Name;
		bool m_UseCache = false;

		std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSPIRV;
		std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSPIRV;

		std::unordered_map<GLenum, std::string> m_OpenGLSourceCode;

		inline static std::unordered_map<uint32_t, UniformBufferSpec> s_UniformBuffers;

	};
}
