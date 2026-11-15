
#!/usr/bin/env python3

import argparse
import os
import platform
import shutil
import subprocess
import sys
import venv

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

QT_VERSION = os.environ.get("QT_VERSION", "6.12.0")

QT_DIR = ROOT / ".qt"
TOOLS_DIR = ROOT / ".tools"
VENV_DIR = TOOLS_DIR / "aqt-venv"


def run(command):
    print("\n>", subprocess.list2cmdline([str(x) for x in command])
          if os.name == "nt"
          else " ".join(str(x) for x in command))

    subprocess.run(command, check=True)

def get_qt_root():
    qt_dir = QT_DIR / QT_VERSION

    configs = list(qt_dir.glob("**/Qt6Config.cmake"))

    if len(configs) != 1:
        raise RuntimeError(
            f"Expected one Qt6Config.cmake, found {len(configs)}"
        )

    return configs[0].parent.parent.parent.parent

def detect_platform():
    system = platform.system().lower()
    machine = platform.machine().lower()

    if system == "linux":
        if machine in ("x86_64", "amd64"):
            return "linux", "desktop", "linux_gcc_64"

        if machine in ("aarch64", "arm64"):
            return "linux_arm64", "desktop", "linux_gcc_arm64"

        raise RuntimeError(f"Unsupported Linux architecture: {machine}")

    if system == "windows":
        if machine not in ("amd64", "x86_64"):
            raise RuntimeError(
                f"Unsupported Windows architecture: {machine}"
            )

        return "windows", "desktop", "win64_msvc2022_64"

    if system == "darwin":
        if machine not in ("arm64", "x86_64"):
            raise RuntimeError(
                f"Unsupported macOS architecture: {machine}"
            )

        return "mac", "desktop", "clang_64"

    raise RuntimeError(f"Unsupported operating system: {system}")


def get_venv_python():
    if os.name == "nt":
        return VENV_DIR / "Scripts" / "python.exe"

    return VENV_DIR / "bin" / "python"


def get_aqt_executable():
    if os.name == "nt":
        return VENV_DIR / "Scripts" / "aqt.exe"

    return VENV_DIR / "bin" / "aqt"


def prepare_aqt():
    python = get_venv_python()

    if not python.exists():
        print("Creating aqtinstall virtual environment...")
        TOOLS_DIR.mkdir(parents=True, exist_ok=True)

        venv.EnvBuilder(with_pip=True).create(VENV_DIR)

    aqt = get_aqt_executable()

    if not aqt.exists():
        print("Installing aqtinstall...")

        run([
            python,
            "-m",
            "pip",
            "install",
            "--upgrade",
            "aqtinstall>=3.3.0,<4"
        ])

    return aqt


def install_qt(force=False):
    os_name, target, arch = detect_platform()

    qt_root = QT_DIR / QT_VERSION / arch
    qt_config = qt_root / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake"

    print(f"Operating system : {os_name}")
    print(f"Qt version       : {QT_VERSION}")
    print(f"Architecture     : {arch}")
    print(f"Installation    : {qt_root}")

    if qt_config.exists() and not force:
        print("\nQt SDK already installed. Skipping download.")
        return qt_root

    if qt_root.exists():
        print("Removing incomplete or previous Qt installation...")
        shutil.rmtree(qt_root)

    aqt = prepare_aqt()

    command = [
        aqt,
        "install-qt",
        os_name,
        target,
        QT_VERSION,
        arch,
        "--outputdir",
        QT_DIR
    ]

    run(command)

    
    # Find Qt6Config.cmake instead of assuming its location.
    config_files = list(QT_DIR.glob(
        f"{QT_VERSION}/**/Qt6Config.cmake"
    ))

    if not config_files:
        raise RuntimeError(
            f"Qt installation finished, but Qt6Config.cmake "
            f"was not found under {QT_DIR / QT_VERSION}"
        )

    if len(config_files) > 1:
        raise RuntimeError(
            f"Multiple Qt installations found: {config_files}"
        )

    qt_config = config_files[0]
    qt_root = qt_config.parent.parent.parent.parent

    print(f"\nQt configuration: {qt_config}")
    print(f"Qt SDK root: {qt_root}")
    print("\nQt installation completed successfully!")

    return qt_root


def main():
    parser = argparse.ArgumentParser(
        description="Automatically install the Qt SDK."
    )

    parser.add_argument(
        "--force",
        action="store_true",
        help="Reinstall Qt even if it is already installed."
    )

    args = parser.parse_args()

    qt_root = install_qt(args.force)

    print(f"\nQt6_ROOT={qt_root}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"\nERROR: {error}", file=sys.stderr)
        sys.exit(1)
