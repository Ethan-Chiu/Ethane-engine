#include "ethpch.h"
#include "OpenGLPipeline.h"

#include <glad/glad.h>

namespace Ethane {
	
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:		return GL_FLOAT;
		case ShaderDataType::Float2:	return GL_FLOAT;
		case ShaderDataType::Float3:	return GL_FLOAT;
		case ShaderDataType::Float4:	return GL_FLOAT;
		case ShaderDataType::Mat3:		return GL_FLOAT;
		case ShaderDataType::Mat4:		return GL_FLOAT;
		case ShaderDataType::Int:		return GL_INT;
		case ShaderDataType::Int2:		return GL_INT;
		case ShaderDataType::Int3:		return GL_INT;
		case ShaderDataType::Int4:		return GL_INT;
		case ShaderDataType::Bool:		return GL_BOOL;
		}

		ETH_CORE_ASSERT(false, "Unknown ShaderDataType!")
			return 0;
	}

	OpenGLPipeline::OpenGLPipeline(const PipelineSpecification& spec)
		:m_Specification(spec)
	{
		Invalidate();
	}

	OpenGLPipeline::~OpenGLPipeline()
	{
		GLuint rendererID = m_VertexArrayRendererID;
		// Renderer::Submit([rendererID]()
		// {
		glDeleteVertexArrays(1, &rendererID);
		// });
	}

	void OpenGLPipeline::Invalidate()
	{
		ETH_PROFILE_FUNCTION();

		ETH_CORE_ASSERT(m_Specification.Layout.GetElements().size(), "Layout is empty!");

		// Ref<OpenGLPipeline> instance = this;
		// Renderer::Submit([instance]() mutable
		// { 
		if (m_VertexArrayRendererID)
			glDeleteVertexArrays(1, &m_VertexArrayRendererID);

		glCreateVertexArrays(1, &m_VertexArrayRendererID);

		const auto& layout = m_Specification.Layout;
		uint32_t attribIndex = 0;
		for (const auto& element : layout)
		{
			// glEnableVertexArrayAttrib(m_VertexArrayRendererID, attribIndex);
			switch (element.Type)
			{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			{
				glEnableVertexAttribArray(attribIndex);
				glVertexAttribPointer(attribIndex,
					element.GetComponentCount(),
					ShaderDataTypeToOpenGLBaseType(element.Type),
					element.Normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)element.Offset);
				break;
			}
			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
			case ShaderDataType::Bool:
			{
				glEnableVertexAttribArray(attribIndex);
				glVertexAttribIPointer(attribIndex,
					element.GetComponentCount(),
					ShaderDataTypeToOpenGLBaseType(element.Type),
					layout.GetStride(),
					(const void*)element.Offset);
				break;
			}
			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				uint8_t count = element.GetComponentCount();
				for (uint8_t i = 0; i < count; i++)
				{
					glEnableVertexAttribArray(attribIndex);
					glVertexAttribPointer(attribIndex,
						count,
						ShaderDataTypeToOpenGLBaseType(element.Type),
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(element.Offset + sizeof(float) * count * i));
					glVertexAttribDivisor(attribIndex, 1);
				}
				break;
			}
			default:
				ETH_CORE_ASSERT(false, "Unknown ShaderDataType!");
			}
			attribIndex++;
		}
		glBindVertexArray(0);
	}

}