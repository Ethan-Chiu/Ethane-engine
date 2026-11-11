"""Locates (or installs) the Vulkan SDK."""

import os
import re
import subprocess
from pathlib import Path
from typing import Optional, Tuple

import Config
import Utils
from Config import Vulkan as cfg


def header_version(sdk: Path) -> Optional[Tuple[int, ...]]:
    """(major, minor, patch) from the SDK's vulkan_core.h, or None if it isn't an SDK."""
    header = sdk / "include" / "vulkan" / "vulkan_core.h"
    if not header.is_file():
        return None
    text = header.read_text(errors="ignore")
    patch = re.search(r"#define VK_HEADER_VERSION (\d+)", text)
    complete = re.search(r"VK_HEADER_VERSION_COMPLETE VK_MAKE_API_VERSION\(\s*\d+\s*,\s*(\d+)\s*,\s*(\d+)", text)
    if not (patch and complete):
        return None
    return int(complete.group(1)), int(complete.group(2)), int(patch.group(1))


def _candidates():
    env = os.environ.get("VULKAN_SDK")
    if env:
        yield Path(env), "VULKAN_SDK"
    # Installed SDKs, newest first (the installer's VULKAN_SDK may not be visible to this shell yet)
    if cfg.default_root.is_dir():
        versions = [d for d in cfg.default_root.iterdir() if d.is_dir() and Utils.parse_version(d.name)]
        for d in sorted(versions, key=lambda d: Utils.parse_version(d.name), reverse=True):
            yield (d / "macOS" if Config.IS_MAC else d), "default install location"


def find_sdk():
    """(path, version, how it was found) of the first valid SDK, or None."""
    for path, source in _candidates():
        version = header_version(path)
        if version:
            return path, version, source
        if source == "VULKAN_SDK":
            Utils.warn(f"VULKAN_SDK={path} is not a Vulkan SDK (no include/vulkan/vulkan_core.h); looking elsewhere")
    return None


def _install():
    if Config.IS_WINDOWS:
        installer = Utils.download(cfg.url, Config.DOWNLOAD_DIR / cfg.url.split("/")[-1])
        Utils.info("Running the Vulkan SDK installer (keep the default install location)...")
        subprocess.run([str(installer)])
        return
    archive = Utils.download(cfg.url, Config.DOWNLOAD_DIR / cfg.url.split("/")[-1])
    unpacked = Config.DOWNLOAD_DIR / f"vulkansdk-macos-{cfg.version}"
    if not unpacked.exists():
        Utils.extract(archive, unpacked)
    apps = sorted(unpacked.glob("*.app"))
    if not apps:
        raise Utils.SetupError(f"no installer .app found in {unpacked}")
    Utils.info(f"Opening {apps[0].name}; keep the default location ({cfg.default_root / cfg.version}).")
    Utils.info("Setup continues when the installer quits.")
    subprocess.run(["open", "-W", str(apps[0])])


def ensure() -> Path:
    """Returns the SDK path (the value for VULKAN_SDK), installing the SDK if needed and accepted."""
    found = find_sdk()
    if not found:
        Utils.warn(f"No Vulkan SDK found (checked VULKAN_SDK and {cfg.default_root}).")
        if not Utils.ask(f"Download and install Vulkan SDK {cfg.version}?"):
            raise Utils.SetupError(f"the Vulkan SDK is required: https://vulkan.lunarg.com/sdk/home ({cfg.version})")
        _install()
        found = find_sdk()
        if not found:
            raise Utils.SetupError("still no Vulkan SDK found after installing; rerun Setup once it's installed")

    path, version, source = found
    tested = Utils.parse_version(cfg.version)[:3]
    version_text = ".".join(map(str, version))
    if version < tested:
        Utils.warn(f"Vulkan SDK {version_text} at {path} is older than the tested {cfg.version}; the build may fail")
    else:
        Utils.ok(f"Vulkan SDK {version_text} at {path} ({source})")
    return path
