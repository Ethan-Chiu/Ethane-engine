#pragma once

#include <memory>
#include "core.h"
#include "spdlog/spdlog.h"

namespace Ethane {

	class ETHANE_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger()
		{
			return s_CoreLogger;
		}

		inline static std::shared_ptr<spdlog::logger>& GetClientLogger()
		{
			return s_ClientLogger;
		}

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
	
}


//core log macros
#define ETH_CORE_TRACE (...) ::ETHANE::Log::GetCoreLoader()->trace(__VA_ARGS__)
#define ETH_CORE_INFO (...)  ::ETHANE::Log::GetCoreLoader()->info(__VA_ARGS__)
#define ETH_CORE_WARN (...)  ::ETHANE::Log::GetCoreLoader()->warn(__VA_ARGS__)
#define ETH_CORE_ERROR (...) ::ETHANE::Log::GetCoreLoader()->error(__VA_ARGS__)
#define ETH_CORE_FATAL (...) ::ETHANE::Log::GetCoreLoader()->fatal(__VA_ARGS__)

//client log macros
#define ETH_TRACE (...) ::ETHANE::Log::GetClientLoader()->trace(__VA_ARGS__)
#define ETH_INFO (...)  ::ETHANE::Log::GetClientLoader()->info(__VA_ARGS__)
#define ETH_WARN (...)  ::ETHANE::Log::GetClientLoader()->warn(__VA_ARGS__)
#define ETH_ERROR (...) ::ETHANE::Log::GetClientLoader()->error(__VA_ARGS__)
#define ETH_FATAL (...) ::ETHANE::Log::GetClientLoader()->fatal(__VA_ARGS__)
