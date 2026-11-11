"""Builds the CMake-based dependencies (assimp, yaml-cpp) out of source into EthaneEngine/vendor/build.

Each library is configured in build/cmake/<lib>-<Config> and installed into build/<Config>
(include/, lib/, bin/), which Dependencies.lua points premake at. A stamp file records the submodule
commit and CMake options of each install, so a library is rebuilt only when one of them changes.
"""

import shutil

import Utils
from Config import Deps as cfg


def _stamp_text(source, args):
    commit = Utils.output(["git", "-C", source, "rev-parse", "HEAD"]) or "unknown"
    return f"{commit}\n" + "\n".join(args) + "\n"


def build(cmake: str, rebuild=()):
    """Builds libraries whose install is missing or stale; `rebuild` names libraries to rebuild from scratch."""
    unknown = set(rebuild) - set(cfg.libraries)
    if unknown:
        raise Utils.SetupError(f"unknown dependency: {', '.join(sorted(unknown))} (known: {', '.join(cfg.libraries)})")

    for name, lib_args in cfg.libraries.items():
        source = cfg.vendor_dir / name
        if not (source / "CMakeLists.txt").is_file():
            raise Utils.SetupError(f"{source} is empty; the submodules aren't checked out")

        for config in cfg.configs:
            prefix = cfg.build_dir / config
            tree = cfg.build_dir / "cmake" / f"{name}-{config}"
            stamp = prefix / f".{name}.stamp"
            args = cfg.common_args + lib_args
            expected = _stamp_text(source, args)

            if name in rebuild:
                shutil.rmtree(tree, ignore_errors=True)
            elif stamp.is_file() and stamp.read_text() == expected:
                Utils.ok(f"{name} ({config}) is up to date")
                continue

            Utils.info(f"Building {name} ({config})...")
            stamp.unlink(missing_ok=True)
            Utils.run([cmake, "-S", source, "-B", tree, f"-DCMAKE_BUILD_TYPE={config}",
                       f"-DCMAKE_INSTALL_PREFIX={prefix}", *args])
            Utils.run([cmake, "--build", tree, "--config", config, "--parallel"])
            Utils.run([cmake, "--install", tree, "--config", config, "--prefix", prefix])
            stamp.write_text(expected)
            Utils.ok(f"{name} ({config}) installed into {prefix}")
