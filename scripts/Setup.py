import os
import subprocess
import sys

import CheckPython
import Config
# Make sure everything we need is installed
CheckPython.ValidatePackages()

import Vulkan
import Premake
import Cmake
import Utils
from PostProcess import AssimpBuild, YamlCppBuild

argument1 = sys.argv[1] if len(sys.argv) > 1 else None
build_deps = False
argument_rest = None
if argument1 == "build":
    build_deps = True
    argument_rest = sys.argv[2:] if len(sys.argv) > 2 else None

# Change from Scripts directory to root
os.chdir('../')

if (not Vulkan.CheckVulkanSDK()):
    print("Vulkan SDK not installed.")
print("----------------------------------------")

# if (not Vulkan.CheckVulkanSDKDebugLibs()):
#     print("Vulkan SDK debug libs not found.")
#     quit()
# print("----------------------------------------")

if (not Premake.check_premake_installed()):
    print("Premake not installed. ")
    quit()
print("----------------------------------------")

if (not Cmake.check_cmake_installed()):
    print("Cmake not installed. ")
print("----------------------------------------")

print("Running premake...")
for action in Config.premake_config.actions:
    premake_path = Utils.filename_in_dir('premake', Config.premake_config.install_dir)
    if premake_path is None:
        print("Premake executable not found.")
        quit()
    print(f"Running: {premake_path.absolute()} {action}")
    subprocess.call([str(premake_path.absolute()), action])
print("----------------------------------------")


if build_deps: 
    print("Post processing...")
    build_all = argument_rest is None
    assimp_task = AssimpBuild() 
    yaml_cpp_task = YamlCppBuild()
    if build_all or assimp_task.get_key() in argument_rest:
        assimp_task.process()
        print("----------------------------------------")
    if build_all or yaml_cpp_task.get_key() in argument_rest:
        yaml_cpp_task.process()
        print("----------------------------------------")

input("Program ended. Press [Enter] to close ... ")
