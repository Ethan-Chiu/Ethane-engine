import os
import subprocess

import Config

def assimp_build():
    root_dir = os.curdir
    print("Build assimp")
    os.chdir("EthaneEngine/vendor/assimp")
    subprocess.run(["cmake", "CMakeLists.txt"])
    subprocess.run(["cmake", "--build", "."])
    os.chdir(root_dir)