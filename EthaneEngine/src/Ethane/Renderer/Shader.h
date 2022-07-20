#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Ethane {

	class Shader
	{
	public:
		virtual ~Shader() = default;

		// virtual void SetInt(const std::string& name, const int value) = 0;
		// virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;
		// virtual void SetFloat(const std::string& name, const float value) = 0;
		// virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		// virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		// virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;


		//Uniform
		virtual uint32_t GetUniformBufferIndex(uint32_t bindingPoint) = 0;
		virtual void SetUniformBuffer(uint32_t uboIndex, const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void SetUniformBufferByBindingPoint(uint32_t bindingPoint, const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual uint32_t GetUniformBufferIndex(const std::string& name) = 0; // test
		virtual void SetUniformBufferByName(const std::string& name, const void* data, uint32_t size) =  0; // test


		virtual const std::string& GetName() const = 0;

		std::string ReadFile(const std::string& filepath);

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

}
