#pragma once

#include "entt.hpp"

#include "Ethane/Core/Timestep.h"

namespace Ethane {

	class Scene {
	public:
		Scene();
		~Scene();

		entt::entity CreateEntity();

		void OnUpdate(Timestep ts);

		entt::registry& Reg() { return m_Registry; }
	private:
		entt::registry m_Registry;
	};

}