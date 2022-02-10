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
	include "EthaneEngine/vendor/Box2D"

group ""


-------------------------------------------------------------------------------
-- Ethane Engine ( main library )
-------------------------------------------------------------------------------
project "EthaneEngine"
	location "EthaneEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "ethpch.h"
	pchsource "%{prj.name}/src/ethpch.cpp"

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",
		"%{prj.name}/vendor/entt/include/**.hpp",

		"%{prj.name}/vendor/VulkanMemoryAllocator/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuiNodeEditor}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.Box2D}",
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp",
		"opengl32.lib",
		"Box2D",
		"%{Library.Vulkan}",
		"%{Library.VulkanUtils}",
	}

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"ETH_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "on"
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}


-------------------------------------------------------------------------------
-- Sandbox
-------------------------------------------------------------------------------
project "Sandbox"
	
	location "Sandbox"
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
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImGuiNodeEditor}",
		"%{IncludeDir.Assimp}"
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
		links
		{
			"EthaneEngine/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../EthaneEngine/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../EthaneEngine/vendor/VulkanSDK/Bin/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"


-------------------------------------------------------------------------------
-- Ethane Editor
-------------------------------------------------------------------------------
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
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.ImGuiNodeEditor}",
		"%{IncludeDir.Assimp}"
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
		links
		{
			"EthaneEngine/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../EthaneEngine/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../EthaneEngine/vendor/VulkanSDK/Bin/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"
		links
		{
			"EthaneEngine/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../EthaneEngine/vendor/assimp/bin/Release/assimp-vc141-mt.dll" "%{cfg.targetdir}"',
		}

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"
		links
		{
			"EthaneEngine/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../EthaneEngine/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
		}
