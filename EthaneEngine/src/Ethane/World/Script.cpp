#include "ethpch.h"
#include "Ethane/World/Script.h"

namespace Ethane {

	ScriptRegistry& ScriptRegistry::Global()
	{
		static ScriptRegistry registry;
		return registry;
	}

	std::unique_ptr<Script> ScriptRegistry::Create(std::string_view name) const
	{
		auto it = m_Factories.find(std::string(name));
		return it != m_Factories.end() ? it->second() : nullptr;
	}

	bool ScriptRegistry::Contains(std::string_view name) const
	{
		return m_Factories.contains(std::string(name));
	}

	std::vector<std::string> ScriptRegistry::Names() const
	{
		std::vector<std::string> names;
		for (const auto& [name, factory] : m_Factories)
			names.push_back(name);
		std::sort(names.begin(), names.end());
		return names;
	}

}
