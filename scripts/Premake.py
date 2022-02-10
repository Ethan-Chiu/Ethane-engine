import os
import Utils

from io import BytesIO
from urllib.request import urlopen
from zipfile import ZipFile

PREMAKE_INSTALLER_URL = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta1/premake-5.0.0-beta1-windows.zip"
OUTPUT_DIRECTORY = "vendor/premake/bin"
PREMAKE_EXE_PATH = f"{OUTPUT_DIRECTORY}/premake5.exe"

def InstallPremake():
    print('Downloading {} to {}'.format(PREMAKE_INSTALLER_URL, PREMAKE_EXE_PATH))
    with urlopen(PREMAKE_INSTALLER_URL) as zipresp:
        with ZipFile(BytesIO(zipresp.read())) as zfile:
            zfile.extractall(OUTPUT_DIRECTORY)
    print("premake5.exe downloaded at: ", PREMAKE_EXE_PATH)
    return True

def InstallPremakePrompt():
    print("Would you like to install the Premake.exe?")
    install = Utils.YesOrNo()
    if (install):
        return InstallPremake()
    else:
        return False

def CheckPremakeExe():
    if (not os.path.isfile(PREMAKE_EXE_PATH)):
        print("You don't have the premake installed!")
        return InstallPremakePrompt()

    print(f"Correct Premake.exe located at {PREMAKE_EXE_PATH}")
    return True