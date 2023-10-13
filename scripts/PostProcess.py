import os
import subprocess

import Config
import Utils


class ProcessTask:
    def __init__(self, key):
        self.key = key

    def get_key() -> str:
        return self.key

    def process(self):
        # Implement the processing logic in the derived classes
        pass

class AssimpBuild(ProcessTask):
    def __init__(self):
        super().__init__("assimp")

    def process(self):
        root_dir = os.getcwd()
        print(f"Processing {self.key}")
        os.chdir("EthaneEngine/vendor/assimp")
        subprocess.run(["cmake", "-DBUILD_SHARED_LIBS=OFF", "-DCMAKE_BUILD_TYPE=Debug", "CMakeLists.txt"])
        subprocess.run(["cmake", "--build", ".", "--config", "Debug"])
        Utils.CheckMarkMsg("Build assimp debug")
        print("----------------------------------------")
        subprocess.run(["cmake", "-DBUILD_SHARED_LIBS=OFF", "-DCMAKE_BUILD_TYPE=Release", "CMakeLists.txt"])
        subprocess.run(["cmake", "--build", ".", "--config", "Release"])
        Utils.CheckMarkMsg("Build assimp release")
        os.chdir(root_dir)

class YamlCppBuild(ProcessTask):
    def __init__(self):
        super().__init__("yamlcpp")

    def process(self):
        root_dir = os.getcwd()
        print("Build yaml cpp")
        os.chdir("EthaneEngine/vendor/yaml-cpp")
        if not os.path.exists("./build"):
            os.makedirs("./build")
        os.chdir("./build")
        subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Debug", ".."])
        subprocess.run(["cmake", "--build", ".", "--config", "Debug"])
        Utils.CheckMarkMsg("Build yaml cpp debug")
        print("----------------------------------------")
        subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Release", ".."])
        subprocess.run(["cmake", "--build", ".", "--config", "Release"])
        Utils.CheckMarkMsg("Build yaml cpp release")
        os.chdir(root_dir) 
    
