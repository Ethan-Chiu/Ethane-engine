import os
import subprocess

import CheckPython
import Config
# Make sure everything we need is installed
CheckPython.ValidatePackages()

import Vulkan
import Premake
import Cmake
import Utils
import PostProcess

# Change from Scripts directory to root
os.chdir('../')

if (not Vulkan.CheckVulkanSDK()):
    print("Vulkan SDK not installed.")
    quit()
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
    quit()
print("----------------------------------------")

print("Running premake...")
subprocess.call([Utils.filename_in_dir('premake', Config.premake_config.install_dir), Config.premake_config.action])
print("----------------------------------------")


print("Post processing...")
PostProcess.assimp_build()
print("----------------------------------------")

input("Program ended. Press [Enter] to close ... ")