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
IncludeDir["GLFW"] = "%{wks.location}/EthaneEngine/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/EthaneEngine/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/EthaneEngine/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/EthaneEngine/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/EthaneEngine/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/EthaneEngine/vendor/entt/include"
IncludeDir["yaml_cpp"] =  "%{wks.location}/EthaneEngine/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] =  "%{wks.location}/EthaneEngine/vendor/ImGuizmo"

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
		"%{IncludeDir.glm}"
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

