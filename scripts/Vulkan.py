import os
import sys
from pathlib import Path

import Config
import Utils

from colorama import Back, Style

VULKAN_SDK = os.environ.get("VULKAN_SDK")


def InstallVulkanSDK():
    cfg = Config.vulkan_config
    vk_installer = LocateVulkanInstaller()
    if not vk_installer:
        print(f"Downloading {cfg.installer_url} to {cfg.filepath}")
        if cfg.filepath.exists():
            print("File already exists.")
        else:
            if cfg.filetype == 'dmg':
                Utils.download_file(cfg.installer_url, cfg.filepath)
            elif cfg.filetype == 'zip':
                Utils.download_archive(cfg.installer_url, cfg.install_dir)
            else:
                print(f"Vulkan SDK filetype={cfg.filetype} not supported")
                quit()
            print("Done!")
        vk_installer = LocateVulkanInstaller()
    else:
        print(f"Vulkan installer already located at {vk_installer}")

    print("Running Vulkan SDK installer...")
    if vk_installer:
        Utils.open_file(vk_installer, cfg.opener)
    else:
        print(f"Please go to {cfg.install_dir} and run the Vulkan SDK installer application.")
        return False
    print(
        "Please set the environment variable VULKAN_SDK to the path of the Vulkan SDK"
    )
    return True


def LocateVulkanInstaller():
    cfg = Config.vulkan_config
    vk_installer = None
    if sys.platform == "darwin":
        vk_installer = Utils.filetype_in_dir(".app", cfg.install_dir)

    return vk_installer


def InstallVulkanPrompt():
    cfg = Config.vulkan_config
    print(f"Would you like to install the Vulkan SDK({cfg.vulkan_sdk_version})?")
    install = Utils.YesOrNo()
    if install:
        return InstallVulkanSDK()
    else:
        return False


def CheckVulkanSDK():
    cfg = Config.vulkan_config
    if VULKAN_SDK is None:
        print("You don't have the Vulkan SDK installed!")
        return InstallVulkanPrompt()
    elif cfg.vulkan_sdk_version not in VULKAN_SDK:
        vulkan_sdk_path = Path(VULKAN_SDK)
        if not vulkan_sdk_path.exists():
            print(
                "Your Vulkan SDK path is invalid! You don't have the Vulkan SDK installed!"
            )
            return InstallVulkanPrompt()
        print(f"Located Vulkan SDK at {VULKAN_SDK}")
        print(f"You don't have the lastest supported version {cfg.vulkan_sdk_version}")
        if any(
            compatible_version in VULKAN_SDK
            for compatible_version in cfg.vulkan_sdk_compatible_versions
        ):
            print("Use compatible version.")
        else:
            print(
                f"You don't have the correct Vulkan SDK version! (ETHANE requires {cfg.vulkan_sdk_version})"
            )
            return InstallVulkanPrompt()

    Utils.CheckMarkMsg(f"Correct Vulkan SDK located at {VULKAN_SDK}")
    return True


def CheckVulkanSDKDebugLibs():
    shadercdLib = Path(f"{VULKAN_SDK}/Lib/shaderc_sharedd.lib")
    if not shadercdLib.exists():
        print(
            f"{Style.BRIGHT}{Back.YELLOW}Warning: No Vulkan SDK debug libs found. (Checked {shadercdLib})"
        )
        print(f"{Back.RED}Debug builds are not possible.{Style.RESET_ALL}")
        return False
    return True
