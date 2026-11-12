-- Dear ImGui as a static library, from the unmodified upstream submodule (EthaneEngine/vendor/imgui,
-- docking branch). Lives here rather than in the submodule, and puts its generated project files in
-- EthaneEngine/vendor/build/projects (gitignored), so the submodule stays clean.
-- The platform / renderer backends are compiled into the engine (Ethane/ImGui/ImGuiBuild.cpp).
project "ImGui"
    location "../EthaneEngine/vendor/build/projects/ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "../EthaneEngine/vendor/imgui/imconfig.h",
        "../EthaneEngine/vendor/imgui/imgui.h",
        "../EthaneEngine/vendor/imgui/imgui.cpp",
        "../EthaneEngine/vendor/imgui/imgui_draw.cpp",
        "../EthaneEngine/vendor/imgui/imgui_internal.h",
        "../EthaneEngine/vendor/imgui/imgui_tables.cpp",
        "../EthaneEngine/vendor/imgui/imgui_widgets.cpp",
        "../EthaneEngine/vendor/imgui/imgui_demo.cpp",
        "../EthaneEngine/vendor/imgui/misc/cpp/imgui_stdlib.h",
        "../EthaneEngine/vendor/imgui/misc/cpp/imgui_stdlib.cpp", -- InputText for std::string
        "../EthaneEngine/vendor/imgui/imstb_rectpack.h",
        "../EthaneEngine/vendor/imgui/imstb_textedit.h",
        "../EthaneEngine/vendor/imgui/imstb_truetype.h",
    }

    filter "system:windows"
        systemversion "latest"

    filter "system:macosx"
        pic "On"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release or configurations:Dist"
        runtime "Release"
        optimize "on"
