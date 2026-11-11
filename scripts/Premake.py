"""Downloads premake and generates the project files."""

import os
from pathlib import Path

import Config
import Utils
from Config import Premake as cfg


def _shown(path: Path) -> str:
    return str(path.relative_to(Config.ROOT)) if path.is_relative_to(Config.ROOT) else str(path)


def ensure() -> Path:
    if cfg.executable.is_file():
        Utils.ok(f"premake at {_shown(cfg.executable)}")
        return cfg.executable
    Utils.warn(f"premake not found at {_shown(cfg.executable)}")
    if not Utils.ask(f"Download premake {cfg.version}?"):
        raise Utils.SetupError(f"premake is required: {cfg.url}")
    archive = Utils.download(cfg.url, Config.DOWNLOAD_DIR / cfg.url.split("/")[-1])
    Utils.extract(archive, cfg.install_dir)
    if not cfg.executable.is_file():
        raise Utils.SetupError(f"{archive.name} did not contain {cfg.executable.name}")
    Utils.ok(f"premake {cfg.version} installed at {_shown(cfg.executable)}")
    return cfg.executable


def generate(premake: Path, vulkan_sdk: Path, actions):
    # Dependencies.lua reads VULKAN_SDK, and the Xcode projects keep its value: pass the SDK that
    # was found so this works without sourcing the SDK's setup-env.sh first
    env = dict(os.environ, VULKAN_SDK=str(vulkan_sdk))
    for action in actions:
        Utils.run([premake, action], cwd=Config.ROOT, env=env)
        Utils.ok(f"Generated {action} projects")
