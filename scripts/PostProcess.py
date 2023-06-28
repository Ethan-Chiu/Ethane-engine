import os
import subprocess

import Config
import Utils

def assimp_build():
    root_dir = os.getcwd()
    print("Build assimp")
    os.chdir("EthaneEngine/vendor/assimp")
    subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Debug", "CMakeLists.txt"])
    subprocess.run(["cmake", "--build", ".", "--config", "Debug"])
    Utils.CheckMarkMsg("Build assimp debug")
    print("----------------------------------------")
    subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Release", "CMakeLists.txt"])
    subprocess.run(["cmake", "--build", ".", "--config", "Release"])
    Utils.CheckMarkMsg("Build assimp release")
    os.chdir(root_dir)

def yaml_cpp_build():
    root_dir = os.getcwd()
    print("Build yaml cpp")
    os.chdir("EthaneEngine/vendor/yaml-cpp")
    subprocess.run(["mkdir", "build"], shell=True)
    os.chdir("build")
    subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Debug", ".."])
    subprocess.run(["cmake", "--build", ".", "--config", "Debug"])
    Utils.CheckMarkMsg("Build yaml cpp debug")
    print("----------------------------------------")
    subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Release", ".."])
    subprocess.run(["cmake", "--build", ".", "--config", "Release"])
    Utils.CheckMarkMsg("Build yaml cpp release")
    os.chdir(root_dir)