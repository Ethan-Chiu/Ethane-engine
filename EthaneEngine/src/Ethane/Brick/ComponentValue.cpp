#include "ethpch.h"
#include "Ethane/Brick/ComponentValue.h"

#include <new>

namespace Ethane {

	ComponentValue::ComponentValue(const TypeInfo& type)
		: m_Type(&type), m_Data(::operator new(type.Size, std::align_val_t(type.Align)))
	{
	}

	ComponentValue::~ComponentValue()
	{
		m_Type->Destroy(m_Data);
		::operator delete(m_Data, std::align_val_t(m_Type->Align));
	}

	std::shared_ptr<ComponentValue> ComponentValue::MakeDefault(const TypeInfo& type)
	{
		std::shared_ptr<ComponentValue> value(new ComponentValue(type));
		type.Construct(value->m_Data);
		return value;
	}

	std::shared_ptr<ComponentValue> ComponentValue::MakeCopy(const TypeInfo& type, const void* source)
	{
		std::shared_ptr<ComponentValue> value(new ComponentValue(type));
		type.CopyConstruct(value->m_Data, source);
		return value;
	}

}
