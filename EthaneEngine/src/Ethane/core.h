#pragma once

#ifdef ETH_PLATFORM_WINDOWS
	#ifdef ETH_BUILD_DLL
		#define ETHANE_API __declspec(dllexport)
	#else
		#define ETHANE_API __declspec(dllimport)
	#endif
#else
	#error Windows support only
#endif

#ifdef ETH_ENABLE_ASSERTS
	#define ETH_ASSERT(x, ...) { if(!(x)) { ETH_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ETH_CORE_ASSERT(x, ...) { if(!(x)) { ETH_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ETH_ASSERT(x, ...)
	#define ETH_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define ETH_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1) 