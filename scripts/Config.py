import platform
from enum import Enum
from pathlib import Path
from typing import List


def systemNotSupported():
    print(f"{platform.system()} is not officially supported yet")
    quit()


class SystemName(Enum):
    Windows = "Windows"
    Mac = "Darwin"


class CmakeConfig:
    def __init__(self):
        system = platform.system()
        self.install_dir: Path = Path("vendor/cmake")
        if system == SystemName.Windows.value:
            self.installer_url: str = (
                "https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-windows-x86_64.msi"
            )
            self.opener = None
        elif system == SystemName.Mac.value:
            self.installer_url: str = (
                "https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-macos-universal.dmg"
            )
            self.opener = "open"
        else:
            systemNotSupported()


class PremakeConfig:
    def __init__(self):
        system = platform.system()
        self.install_dir = Path("vendor/premake/bin")
        if system == SystemName.Windows.value:
            self.installer_url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip"
            self.actions = ["vs2022", "gmake2"]
        elif system == SystemName.Mac.value:
            self.installer_url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz"
            self.actions = ["xcode4", "gmake2"]
        else:
            systemNotSupported()


class VulkanConfig:
    def __init__(self):
        system = platform.system()
        self.vulkan_sdk_version: str = "1.4.309.0"
        self.vulkan_sdk_compatible_versions: List[str] = ["1.3.204.1", "1.3.250.0"]
        self.install_dir: Path = Path("EthaneEngine/vendor/VulkanSDK")
        if system == SystemName.Windows.value:
            self.installer_url: str = (
                f"https://sdk.lunarg.com/sdk/download/{self.vulkan_sdk_version}/windows/VulkanSDK-{self.vulkan_sdk_version}-Installer.exe"
            )
            self.filepath: Path = self.install_dir / "VulkanSDK.exe"
            self.opener = None
        elif system == SystemName.Mac.value:
            self.filetype: str = "zip"
            self.installer_url: str = (
                f"https://sdk.lunarg.com/sdk/download/{self.vulkan_sdk_version}/mac/vulkansdk-macos-{self.vulkan_sdk_version}.{self.filetype}"
            )
            self.filepath: Path = self.install_dir / f"VulkanSDK.{self.filetype}"
            self.opener = "open"
        else:
            systemNotSupported()


# Create an instance of the Config class
cmake_config = CmakeConfig()
premake_config = PremakeConfig()
vulkan_config = VulkanConfig()
