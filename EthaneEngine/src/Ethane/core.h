#pragma once

#ifdef ETH_PLATFORM_WINDOWS
	#ifdef ETN_BUILD_DLL
		#define ETHANE_API __declspec(dllexport)
	#else
		#define ETHANE_API __declspec(dllimport)
	#endif
#else
	#error Windows support only
#endif

#define BIT(x) (1 << x)