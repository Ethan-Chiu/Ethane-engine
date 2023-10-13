-- EthaneEngine Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/EthaneEngine/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/EthaneEngine/vendor/Glad/include"
IncludeDir["spdlog"] = "%{wks.location}/EthaneEngine/vendor/spdlog/include"
IncludeDir["glm"] = "%{wks.location}/EthaneEngine/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/EthaneEngine/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/EthaneEngine/vendor/entt/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
-- IncludeDir["shaderc"] = "%{wks.location}/EthaneEngine/vendor/shaderc/include"
-- IncludeDir["SPIRV_Cross"] = "%{wks.location}/EthaneEngine/vendor/SPIRV-Cross"
IncludeDir["assimp"] = "%{wks.location}/EthaneEngine/vendor/assimp/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/EthaneEngine/vendor/yaml-cpp/include"
IncludeDir["imgui"] = "%{wks.location}/EthaneEngine/vendor/imgui"
IncludeDir["ImGuizmo"] = "%{wks.location}/EthaneEngine/vendor/ImGuizmo"


LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/lib"
LibraryDir["vendor"] = "%{wks.location}/EthaneEngine/vendor"

LibraryDir["assimp"] = "%{LibraryDir.vendor}/assimp/lib"
LibraryDir["yamlcpp"] = "%{LibraryDir.vendor}/yaml-cpp/build"

-- General
Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils"

-- Debug 
Library["ShaderC_Debug"] = "shaderc_sharedd"
Library["ShaderC_Utils_Debug"] = "shaderc_utild"
Library["SPIRV_Cross_Debug"] = "spirv-cross-cored"
Library["SPIRV_Cross_GLSL_Debug"] = "spirv-cross-glsld"
Library["SPIRV_Tools_Debug"] = "SPIRV-Toolsd"

Library["YamlCppDebug"] = "yaml-cppd"

-- Release 
Library["ShaderC"] = "shaderc_shared"
Library["ShaderC_Utils"] = "shaderc_util"
Library["SPIRV_Cross"] = "spirv-cross-core"
Library["SPIRV_Cross_GLSL"] = "spirv-cross-glsl"
Library["SPIRV_Tools"] = "SPIRV-Tools"

Library["YamlCpp"] = "yaml-cpp"

-- Windows 
Library["AssimpDebug"] = "assimp-vc143-mtd"
Library["Assimp"] = "assimp-vc143-mt"

-- Mac
Library["MacAssimp"] = "assimp"
