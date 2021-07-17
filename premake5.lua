include "Dependencies.lua"

workspace "EthaneEngine"
	architecture "x86_64"
	startproject "Ethane-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


group "Dependencies"
	include "EthaneEngine/vendor/GLFW"
	include "EthaneEngine/vendor/Glad"
	include "EthaneEngine/vendor/imgui"
	include "EthaneEngine/vendor/yaml-cpp"

group ""

include "EthaneEngine"

project "Sandbox"
	
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"EthaneEngine/vendor/spdlog/include",
		"EthaneEngine/src",
		"EthaneEngine/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"EthaneEngine"
	}

	filter "system:windows"
		systemversion "latest"
		
	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"

project "Ethane-Editor"

	location "Ethane-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"EthaneEngine/vendor/spdlog/include",
		"EthaneEngine/src",
		"EthaneEngine/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"EthaneEngine"
	}

	filter "system:windows"
		systemversion "latest"
		
	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"

