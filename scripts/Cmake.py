import subprocess
import os 
from pathlib import Path

import Config
import Utils

def check_cmake_installed():
    cfg = Config.cmake_config
    if (not _is_installed()):
        print("You don't have the cmake installed!")
        return install_cmake_prompt()

    Utils.CheckMarkMsg(f"Correct Cmake located at {cfg.install_dir}")
    return True
    
def _is_installed():
    try:
        subprocess.run(['cmake', '--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def download_cmake():
    cfg = Config.cmake_config
    filename = "cmake." + cfg.installer_url.split(".")[-1]
    filepath = Path(cfg.install_dir).joinpath(filename)
    if not Utils.filename_exists_in_dir("cmake", cfg.install_dir):
        print('Downloading {} to {}'.format(cfg.installer_url, filepath))
        Utils.download_file(cfg.installer_url, filepath)
    Utils.open_file(filepath, cfg.opener)
    if _is_installed():
        print("Cmake installed")
    return True

def install_cmake_prompt():
    print("Would you like to install the cmake?")
    install = Utils.YesOrNo()
    if (install):
        return download_cmake()
    else:
        return False