#include "ethpch.h"
#include "AssetManager.h"

#include "ShaderLibrary.h"

namespace Ethane {

	void AssetManager::Init()
	{
		ShaderLibrary::Init();
	}

	void AssetManager::Shutdown()
	{
		ShaderLibrary::Shutdown();
	}
}