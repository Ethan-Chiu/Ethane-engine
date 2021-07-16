#pragma once

#include "Ethane/Core/Base.h"

#include "Shader.h"
#include "VertexBuffer.h"

namespace Ethane {

	struct PipelineSpecification
	{
		Ref<Shader> Shader;
		VertexBufferLayout Layout;
	};

	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);
	};
}