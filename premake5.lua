workspace "EthaneEngine"
	architecture "x64"
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	startproject "Sandbox"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "EthaneEngine/vendor/GLFW/include"
IncludeDir["Glad"] = "EthaneEngine/vendor/Glad/include"
IncludeDir["ImGui"] = "EthaneEngine/vendor/imgui"

include "EthaneEngine/vendor/GLFW"
include "EthaneEngine/vendor/Glad"
include "EthaneEngine/vendor/imgui"


project "EthaneEngine"

	location "EthaneEngine"
	kind "SharedLib"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "ethpch.h"
	pchsource "EthaneEngine/src/ethpch.cpp"

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"ETH_PLATFORM_WINDOWS",
			"ETH_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "On"


project "Sandbox"
	
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"EthaneEngine/vendor/spdlog/include",
		"EthaneEngine/src"
	}

	links
	{
		"EthaneEngine"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"ETH_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "On"

