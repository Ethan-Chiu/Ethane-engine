"""Sets up EthaneEngine for development: submodules, Vulkan SDK, premake, dependencies, project files.

Run from anywhere, as often as you like; finished steps are skipped:
    python3 scripts/Setup.py              (or scripts/Setup.sh, scripts\\Setup.bat)
    python3 scripts/Setup.py --help
"""

import sys

if sys.version_info < (3, 9):
    sys.exit(f"Setup needs Python 3.9 or newer (this is {sys.version.split()[0]})")

import argparse

import CMake
import Config
import Dependencies
import Premake
import Utils
import Vulkan


def check_platform():
    if not (Config.IS_MAC or Config.IS_WINDOWS):
        raise Utils.SetupError(f"{Config.SYSTEM} isn't supported yet (macOS and Windows are)")


def init_submodules():
    if not (Config.ROOT / ".git").exists():
        Utils.warn("Not a git checkout: can't fetch submodules (clone the repository with git instead)")
        return
    status = Utils.output(["git", "submodule", "status"], cwd=Config.ROOT).splitlines()
    missing = [line.split()[1] for line in status if line.startswith("-")]
    moved = [line.split()[1] for line in status if line.startswith("+")]
    if missing:
        Utils.run(["git", "submodule", "update", "--init", "--recursive", "--", *missing], cwd=Config.ROOT)
    for path in moved:  # left alone: may be deliberate local work
        Utils.warn(f"{path} is not at the commit this repository records (`git submodule update {path}` resets it)")
    Utils.ok(f"{len(status)} submodules checked out")


def parse_args():
    parser = argparse.ArgumentParser(description="Sets up EthaneEngine for development.")
    parser.add_argument("-y", "--yes", action="store_true",
                        help="answer yes to every download / install question")
    parser.add_argument("--no-deps", action="store_true",
                        help="don't build assimp / yaml-cpp (just regenerate project files)")
    parser.add_argument("--rebuild-deps", nargs="*", metavar="LIB",
                        help=f"rebuild dependencies from scratch: {', '.join(Config.Deps.libraries)} (all if none given)")
    parser.add_argument("--action", action="append", metavar="ACTION",
                        help=f"premake action(s) to run (default: {' '.join(Config.Premake.actions)})")
    return parser.parse_args()


def print_next_steps(vulkan_sdk):
    Utils.step("Done")
    if Config.IS_MAC:
        Utils.info("Open EthaneEngine.xcworkspace in Xcode and run the Ethane-Editor scheme")
        Utils.info(f"(Launching from a shell instead needs: source \"{vulkan_sdk.parent / 'setup-env.sh'}\")")
    else:
        Utils.info("Open EthaneEngine.sln in Visual Studio and run Ethane-Editor (F5)")
    Utils.info("Rerun this script after pulling changes to premake files or submodules.")


def main():
    sys.stdout.reconfigure(line_buffering=True)  # keeps output in order with stderr when piped
    args = parse_args()
    Utils.assume_yes = args.yes
    check_platform()

    Utils.step("Submodules")
    init_submodules()

    Utils.step("Vulkan SDK")
    vulkan_sdk = Vulkan.ensure()

    Utils.step("Premake")
    premake = Premake.ensure()

    if args.no_deps:
        Utils.step("Dependencies (skipped: --no-deps)")
    else:
        Utils.step("Dependencies")
        cmake = CMake.ensure()
        rebuild = args.rebuild_deps
        if rebuild is not None and not rebuild:  # --rebuild-deps without names: everything
            rebuild = list(Config.Deps.libraries)
        Dependencies.build(cmake, rebuild or ())

    Utils.step("Project files")
    Premake.generate(premake, vulkan_sdk, args.action or Config.Premake.actions)

    print_next_steps(vulkan_sdk)


if __name__ == "__main__":
    try:
        main()
    except Utils.SetupError as e:
        Utils.error(f"Setup failed: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print()
        sys.exit(130)
