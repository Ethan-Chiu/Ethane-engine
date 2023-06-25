import os
import subprocess

import CheckPython

# Make sure everything we need is installed
CheckPython.ValidatePackages()

import Vulkan
import Premake

import platform

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

if (not Premake.CheckPremakeExe()):
    print("Premake5.exe not installed. ")
    quit()
print("----------------------------------------")

print("Running premake...")
if platform.system() == 'Darwin':
    subprocess.call([Premake.PREMAKE_EXE_PATH, "xcode4"])
else: 
    subprocess.call([Premake.PREMAKE_EXE_PATH, "vs2022"])
print("----------------------------------------")

input("Program ended. Press [Enter] to close ... ")