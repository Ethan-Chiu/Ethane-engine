"""Locates (or installs) CMake, which builds the CMake-based dependencies."""

import subprocess
from pathlib import Path

import Config
import Utils
from Config import CMake as cfg

# Installs that don't necessarily put cmake on PATH
_KNOWN_LOCATIONS = [
    Path("/Applications/CMake.app/Contents/bin/cmake"),
    Path("/opt/homebrew/bin/cmake"),
    Path("/usr/local/bin/cmake"),
] if Config.IS_MAC else [
    Path("C:/Program Files/CMake/bin/cmake.exe"),
]


def find() -> str:
    found = Utils.which("cmake")
    if found:
        return found
    return next((str(p) for p in _KNOWN_LOCATIONS if p.is_file()), "")


def _install():
    if Config.IS_MAC and Utils.which("brew"):
        Utils.run(["brew", "install", "cmake"])
        return
    installer = Utils.download(cfg.url, Config.DOWNLOAD_DIR / cfg.url.split("/")[-1])
    if Config.IS_WINDOWS:
        subprocess.run(["msiexec", "/i", str(installer)])
    else:
        Utils.info("Drag CMake into Applications, then close the window; setup continues afterwards.")
        subprocess.run(["open", "-W", str(installer)])


def ensure() -> str:
    cmake = find()
    if not cmake:
        Utils.warn("CMake not found (needed to build assimp and yaml-cpp).")
        how = "with Homebrew" if Config.IS_MAC and Utils.which("brew") else f"CMake {cfg.version}"
        if not Utils.ask(f"Install {how}?"):
            raise Utils.SetupError("CMake is required to build the dependencies: https://cmake.org/download/")
        _install()
        cmake = find()
        if not cmake:
            raise Utils.SetupError("still no CMake found after installing; rerun Setup once it's installed")
    version = Utils.output([cmake, "--version"]).splitlines()
    Utils.ok(f"{version[0] if version else 'cmake'} at {cmake}")
    return cmake
