#pragma once

#include<memory>

#include "PlatformDetection.h"

// DLL support
/* #ifdef ETH_PLATFORM_WINDOWS
	#if ETH_DYNAMIC_LINK
		#ifdef ETH_BUILD_DLL
			#define ETHANE_API __declspec(dllexport)
		#else
			#define ETHANE_API __declspec(dllimport)
		#endif
	#else
		#define ETHANE_API
	#endif
#else
	#error Windows support only
#endif */

//////////////////////////
#define ETH_ENABLE_ASSERTS

#ifdef ETH_ENABLE_ASSERTS
	#define ETH_ASSERT(x, ...) { if(!(x)) { ETH_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ETH_CORE_ASSERT(x, ...) { if(!(x)) { ETH_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ETH_ASSERT(x, ...)
	#define ETH_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define ETH_BIND_EVENT_FN(fn)  [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); } 

namespace Ethane {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}