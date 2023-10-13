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

group ""
  project "EthaneEngine"

group ""
  project "Ethane-Editor"


-------------------------------------------------------------------------------
-- Ethane Engine ( main library )
-------------------------------------------------------------------------------
project "EthaneEngine"

	location "EthaneEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

  filter "system:windows"
		systemversion "latest"
  filter "system:macosx"
    systemversion "10.15.0:14:0.0"
	filter {}

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchsource "%{prj.name}/src/ethpch.cpp"

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/entt/include/**.hpp",

		-- "%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		-- "%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",
		-- "%{prj.name}/vendor/VulkanMemoryAllocator/**.h",
		-- "%{prj.name}/vendor/VulkanMemoryAllocator/**.cpp"
	}

	externalincludedirs
  {
    "%{IncludeDir.spdlog}",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.Glad}",
    "%{IncludeDir.glm}",
    "%{IncludeDir.stb_image}",
    "%{IncludeDir.entt}",
    "%{IncludeDir.VulkanSDK}",
    "%{IncludeDir.assimp}",
    "%{IncludeDir.yaml_cpp}",
    "%{IncludeDir.imgui}",
  }

	includedirs
	{
		"%{wks.location}/EthaneEngine/src",
	}

	libdirs { "%{LibraryDir.VulkanSDK}" }

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
	}

  filter "action:xcode4"
    pchheader "src/ethpch.h"
  filter "action:not xcode4"
    pchheader "ethpch.h"

	filter "files:EthaneEngine/vendor/ImGuizmo/**.cpp"
    flags { "NoPCH" }


	filter "system:windows"
		defines
		{
			"ETH_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

    links 
    {
			"vulkan-1",
			-- "%{Library.VulkanUtils}",
		}

  filter { "system:windows", "configurations:Debug" }
    libdirs
    {
      "%{LibraryDir.yamlcpp}/Debug",
      "%{LibraryDir.assimp}/Debug"
    }

    links
    {
      "%{Library.ShaderC_Debug}",
      "%{Library.ShaderC_Utils_Debug}",
      "%{Library.SPIRV_Cross_Debug}",
      "%{Library.SPIRV_Cross_GLSL_Debug}",
      "%{Library.SPIRV_Tools_Debug}",
      "%{Library.AssimpDebug}",
      "%{Library.YamlCppDebug}"
    }

  filter { "system:windows", "configurations:Release" }
    libdirs
    {
      "%{LibraryDir.yamlcpp}/Release",
      "%{LibraryDir.assimp}/Release"
    }

    links
    {
      "%{Library.ShaderC}",
      "%{Library.ShaderC_Utils}",
      "%{Library.SPIRV_Cross}",
      "%{Library.SPIRV_Cross_GLSL}",
      "%{Library.SPIRV_Tools}",
      "%{Library.Assimp}",
      "%{Library.YamlCpp}"
    }

  filter { "system:windows", "configurations:Dist" }
    libdirs {
      "%{LibraryDir.yamlcpp}/Release",
      "%{LibraryDir.assimp}/Release"
    }

    links
    {
      "%{Library.ShaderC}",
      "%{Library.ShaderC_Utils}",
      "%{Library.SPIRV_Cross}",
      "%{Library.SPIRV_Cross_GLSL}",
      "%{Library.SPIRV_Tools}",
      "%{Library.Assimp}",
      "%{Library.YamlCpp}"
    }

  filter {}


	filter "system:macosx"
		defines
    {
			"CFG_MACOS",
			"__APPLE__"
		}
		kind "SharedLib"

		libdirs 
    { 
			"%{LibraryDir.yamlcpp}",
			"%{LibraryDir.assimp}"
		}

		links 
    {
			"OpenGL.framework",
			"Cocoa.framework",
			"IOKit.framework",
      "vulkan",
			"z",
      "%{Library.ShaderC}",
			"%{Library.ShaderC_Utils}",
			"%{Library.SPIRV_Cross}",
			"%{Library.SPIRV_Cross_GLSL}",
			"%{Library.SPIRV_Tools}",
			"%{Library.MacAssimp}",
			"%{Library.YamlCpp}"
		}

    runpathdirs { "%{LibraryDir.VulkanSDK}" }

	filter { "system:macosx", "configurations:Debug" }
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "on"

	filter { "system:macosx", "configurations:Release" }
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"

	filter { "system:macosx", "configurations:Dist" }
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
	cppdialect "C++20"
	staticruntime "off"

  filter "system:windows"
		systemversion "latest"
  filter "system:macosx"
    systemversion "10.15.0:14:0.0"
	filter {}

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp"
	}

	externalincludedirs 
  { 
    "%{wks.location}/EthaneEngine/src",
    "%{IncludeDir.spdlog}",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.Glad}",
    "%{IncludeDir.glm}",
    "%{IncludeDir.stb_image}",
    "%{IncludeDir.entt}",
    "%{IncludeDir.VulkanSDK}",
    "%{IncludeDir.assimp}",
    "%{IncludeDir.yaml_cpp}",
    "%{IncludeDir.imgui}"
  }

  includedirs
  {
  }

	links
	{
		"EthaneEngine"
	}

		
	filter "configurations:Debug"
		defines "ETH_DEBUG"
		runtime "Debug"
		symbols "on"

  filter { "system:windows", "configurations:Debug"}
    postbuildcommands 
    {
      '{COPY} "%{LibraryDir.vendor}/assimp/bin/Debug/%{Library.AssimpDebug}.dll" "%{cfg.targetdir}"'
    }

	filter "configurations:Release"
		defines "ETH_RELEASE"
		runtime "Release"
		optimize "on"

  filter { "system:windows", "configurations:Release"}
    postbuildcommands 
    {
      '{COPY} "%{LibraryDir.vendor}/assimp/bin/Release/%{Library.Assimp}.dll" "%{cfg.targetdir}"'
    }

	filter "configurations:Dist"
		defines "ETH_DIST"
		runtime "Release"
		symbols "on"

  filter { "system:windows", "configurations:Dist"}
    postbuildcommands 
    {
      '{COPY} "%{LibraryDir.vendor}/assimp/bin/Release/%{Library.Assimp}.dll" "%{cfg.targetdir}"'
    }

