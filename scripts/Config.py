import platform
from enum import Enum

class SystemName(Enum):
    Windows = 'Windows'
    Mac = 'Darwin'

class CmakeConfig:
    def __init__(self):
        system = platform.system()
        self.install_dir = "vendor/cmake"
        if system == SystemName.Windows.value:
            self.installer_url = "https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-windows-x86_64.msi"
            self.opener = None
        elif system == SystemName.Mac.value:
            self.installer_url = "https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-macos-universal.dmg"
            self.opener = "open"
        else:
            print(f"{system} is not officially supported yet")
            quit()

class PremakeConfig:
    def __init__(self):
        system = platform.system()
        self.install_dir = "vendor/premake/bin"
        if system == SystemName.Windows.value:
            self.installer_url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip"
            self.action = "xcode4"
        elif system == SystemName.Mac.value:
            self.installer_url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz"
            self.action = "vs2022"
        else:
            print(f"{system} is not officially supported yet")
            quit()


class VulkanConfig:
    def __init__(self):
        system = platform.system()
        self.vulkan_sdk_version = "1.3.250.0"
        self.install_dir = "EthaneEngine/vendor/VulkanSDK"
        if system == SystemName.Windows.value:
            self.installer_url = f'https://sdk.lunarg.com/sdk/download/{self.vulkan_sdk_version}/windows/VulkanSDK-{self.vulkan_sdk_version}-Installer.exe'
            self.filepath = f'{self.install_dir}/VulkanSDK.exe'
            self.opener = None
        elif system == SystemName.Mac.value:
            self.installer_url = f'https://sdk.lunarg.com/sdk/download/{self.vulkan_sdk_version}/mac/vulkansdk-macos-{self.vulkan_sdk_version}.dmg'
            self.filepath = f'{self.install_dir}/VulkanSDK.dmg'
            self.opener = "open"
        else:
            print(f"{system} is not officially supported yet")
            quit()

# Create an instance of the Config class
cmake_config = CmakeConfig()
premake_config = PremakeConfig()
vulkan_config = VulkanConfig()
