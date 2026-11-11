# EthaneEngine

## Introduction

Ethane Game Engine: a Vulkan engine and editor for macOS (Apple Silicon) and Windows.

## Setup

Requirements:

* Python 3.9 or newer
* git
* macOS: Xcode; Windows: Visual Studio 2022

Everything else is found or, after asking, downloaded by the setup script: the Vulkan SDK, premake, CMake,
and the assimp / yaml-cpp builds.

1. Clone the repository (the setup script fetches the submodules if you leave out `--recursive`):

        git clone --recursive https://github.com/Ethan-Chiu/Ethane-engine.git

2. Run the setup script, from any directory:

        scripts/Setup.sh            # macOS
        scripts\Setup.bat           # Windows (or double-click it)

   It checks out the submodules, finds the Vulkan SDK (`VULKAN_SDK`, else the installer's default
   location), downloads premake, builds assimp and yaml-cpp into `EthaneEngine/vendor/build`, and
   generates the project files. Finished steps are skipped, so rerun it whenever premake files or
   submodules change. `--help` lists the options, e.g. `--no-deps` to only regenerate project files,
   `--rebuild-deps` for a clean dependency build, `--yes` to accept every download.

3. Build and run `Ethane-Editor`:
   * macOS: open `EthaneEngine.xcworkspace` and run the `Ethane-Editor` scheme.
   * Windows: open `EthaneEngine.sln` and run `Ethane-Editor` (F5).

## Tests

`Ethane-Tests` holds unit tests for engine code that needs no window or GPU (schema, bricks). Run the
`Ethane-Tests` scheme / project, or from a shell after building Debug on macOS:

    DYLD_FALLBACK_LIBRARY_PATH=bin/Debug-macosx-ARM64/EthaneEngine bin/Debug-macosx-ARM64/Ethane-Tests/Ethane-Tests [filter]

## Design

[docs/Architecture.md](docs/Architecture.md) describes how bricks, worlds and frame packets fit together;
[docs/Roadmap.md](docs/Roadmap.md) tracks progress.

## Note ( For developer ) 

Memory management: unreal Core/HAL

Vulkan Descriptor pool: VulkanImGuiLayer, VulkanMaterial,