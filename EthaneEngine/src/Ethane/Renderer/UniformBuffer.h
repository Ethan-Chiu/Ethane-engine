#pragma once

#include "Ethane/Core/Base.h"

namespace Ethane {

	enum class UniformVariableType
	{
		None = 0, Bool, Int, UInt, Float, Vec2, Vec3, Vec4, Mat3, Mat4,
		IVec2, IVec3, IVec4
	};

	class UniformVariable
	{
	public:
		UniformVariable() = default;
		UniformVariable(std::string name, UniformVariableType type, uint32_t size, uint32_t offset);

		const std::string& GetName() const { return m_Name; }
		UniformVariableType GetType() const { return m_Type; }
		uint32_t GetSize() const { return m_Size; }
		uint32_t GetOffset() const { return m_Offset; }

		static const std::string& UniformTypeToString(UniformVariableType type);
	private:
		std::string m_Name;
		UniformVariableType m_Type = UniformVariableType::None;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
	};

	struct UniformBufferSpec
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<UniformVariable> Uniforms;
	};

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() {}
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
	};

}
