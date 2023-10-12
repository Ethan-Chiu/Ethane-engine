import os
import subprocess
from pathlib import Path
import platform

import Config
import Utils

from colorama import Back, Style

VULKAN_SDK = os.environ.get('VULKAN_SDK')

def InstallVulkanSDK():
    cfg = Config.vulkan_config
    print('Downloading {} to {}'.format(cfg.installer_url, cfg.filepath))
    Utils.download_file(cfg.installer_url, cfg.filepath)
    print("Done!")
    print("Running Vulkan SDK installer...")
    Utils.open_file(cfg.filepath, cfg.opener)
    return True

def InstallVulkanPrompt():
    cfg = Config.vulkan_config
    print(f"Would you like to install the Vulkan SDK({cfg.vulkan_sdk_version})?")
    install = Utils.YesOrNo()
    if (install):
        return InstallVulkanSDK()
    else:
        return False

def CheckVulkanSDK():
    cfg = Config.vulkan_config
    if (VULKAN_SDK is None):
        print("You don't have the Vulkan SDK installed!")
        return InstallVulkanPrompt()
    elif (cfg.vulkan_sdk_version not in VULKAN_SDK):
        print(f"Located Vulkan SDK at {VULKAN_SDK}")
        print(f"You don't have the lastest supported version {cfg.vulkan_sdk_version}")
        if any(compatible_version in VULKAN_SDK for compatible_version in cfg.vulkan_sdk_compatible_versions):
            print("Use compatible version.")
        else:
            print(f"You don't have the correct Vulkan SDK version! (ETHANE requires {cfg.vulkan_sdk_version})")
            return InstallVulkanPrompt()

    Utils.CheckMarkMsg(f"Correct Vulkan SDK located at {VULKAN_SDK}")
    return True


def CheckVulkanSDKDebugLibs():
    shadercdLib = Path(f"{VULKAN_SDK}/Lib/shaderc_sharedd.lib")
    if (not shadercdLib.exists()):
        print(f"{Style.BRIGHT}{Back.YELLOW}Warning: No Vulkan SDK debug libs found. (Checked {shadercdLib})")
        print(f"{Back.RED}Debug builds are not possible.{Style.RESET_ALL}")
        return False
    return True
