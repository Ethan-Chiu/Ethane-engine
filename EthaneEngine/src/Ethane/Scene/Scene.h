#pragma once

#include "entt.hpp"

namespace Ethane {

	class Scene {
	public:
		Scene();
		~Scene();
	private:
		entt::registry m_Registry;
	};

}