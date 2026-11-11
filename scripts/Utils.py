"""Console output, prompts, commands and downloads for Setup.py (standard library only)."""

import os
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Optional


class SetupError(Exception):
    """A step failed; Setup.py prints the message and exits with an error."""


# ---------------------------------------------------------------------------- output

if os.name == "nt":
    os.system("")  # enables ANSI escape sequences in the Windows console
_COLOR = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None


def _paint(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _COLOR else text


def step(title: str):
    print()
    print(_paint("1", f"== {title}"))


def ok(msg: str):
    print(f"{_paint('32', '✓')} {msg}")


def info(msg: str):
    print(f"  {msg}")


def warn(msg: str):
    print(f"{_paint('33', '!')} {msg}")


def error(msg: str):
    print(f"{_paint('31', '✗')} {msg}", file=sys.stderr)


# ---------------------------------------------------------------------------- prompts

assume_yes = False  # --yes


def ask(question: str) -> bool:
    if assume_yes:
        print(f"? {question} [y/n]: y (--yes)")
        return True
    if not sys.stdin.isatty():
        print(f"? {question} [y/n]: n (not interactive; pass --yes to accept)")
        return False
    while True:
        reply = input(f"? {question} [y/n]: ").strip().lower()
        if reply[:1] in ("y", "n"):
            return reply[0] == "y"


# ---------------------------------------------------------------------------- commands

def run(cmd, cwd: Optional[Path] = None, env: Optional[dict] = None):
    """Runs a command with its output shown; raises SetupError if it fails."""
    cmd = [str(c) for c in cmd]
    print(_paint("2", f"$ {' '.join(cmd)}"), flush=True)
    try:
        result = subprocess.run(cmd, cwd=cwd, env=env)
    except FileNotFoundError:
        raise SetupError(f"{cmd[0]} not found")
    if result.returncode != 0:
        raise SetupError(f"command failed (exit code {result.returncode}): {' '.join(cmd)}")


def output(cmd, cwd: Optional[Path] = None) -> str:
    """Runs a command and returns its stdout, or "" if it can't be run or fails."""
    try:
        result = subprocess.run([str(c) for c in cmd], cwd=cwd, capture_output=True, text=True)
    except FileNotFoundError:
        return ""
    return result.stdout.strip() if result.returncode == 0 else ""


def which(program: str) -> str:
    return shutil.which(program) or ""


# ---------------------------------------------------------------------------- downloads

def download(url: str, destination: Path) -> Path:
    """Downloads url to destination (reusing a previous complete download)."""
    if destination.exists():
        info(f"Using previously downloaded {destination}")
        return destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    partial = destination.with_name(destination.name + ".part")
    info(f"Downloading {url}")
    request = urllib.request.Request(url, headers={"User-Agent": "EthaneEngine-Setup"})
    try:
        with urllib.request.urlopen(request) as response, open(partial, "wb") as file:
            total = int(response.headers.get("content-length") or 0)
            done = 0
            while chunk := response.read(1 << 16):
                file.write(chunk)
                done += len(chunk)
                if total and sys.stdout.isatty():
                    print(f"\r  {done / total:6.1%} of {total / (1 << 20):.1f} MB", end="", flush=True)
        if total and sys.stdout.isatty():
            print()
    except OSError as e:
        partial.unlink(missing_ok=True)
        raise SetupError(f"download failed: {url} ({e})")
    partial.replace(destination)
    return destination


def extract(archive: Path, destination: Path):
    destination.mkdir(parents=True, exist_ok=True)
    if archive.name.endswith((".tar.gz", ".tgz")):
        with tarfile.open(archive) as tar:
            if hasattr(tarfile, "data_filter"):  # Python 3.12+ (and security backports)
                tar.extractall(destination, filter="data")
            else:
                tar.extractall(destination)
    else:
        with zipfile.ZipFile(archive) as zf:
            for member in zf.infolist():
                zf.extract(member, destination)
                mode = member.external_attr >> 16  # zipfile drops Unix permissions (e.g. executable bits)
                if mode:
                    os.chmod(destination / member.filename, mode)


def parse_version(text: str):
    """'1.4.309.0' -> (1, 4, 309, 0); non-numeric parts are ignored."""
    return tuple(int(part) for part in text.split(".") if part.isdigit())
