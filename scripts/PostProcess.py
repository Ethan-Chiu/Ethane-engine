import os
import subprocess

import Config

def assimp_build():
    root_dir = os.getcwd()
    print("Build assimp")
    os.chdir("EthaneEngine/vendor/assimp")
    subprocess.run(["cmake", "CMakeLists.txt"])
    subprocess.run(["cmake", "--build", "."])
    os.chdir(root_dir)

def yaml_cpp_build():
    root_dir = os.getcwd()
    print("Build yaml cpp")
    os.chdir("EthaneEngine/vendor/yaml-cpp")
    subprocess.run(["mkdir", "build"], shell=True)
    os.chdir("build")
    subprocess.run(["cmake", ".."])
    subprocess.run(["cmake", "--build", "."])
    os.chdir(root_dir)