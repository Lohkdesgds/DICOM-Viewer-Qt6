
#!/usr/bin/env python3

import os
import platform
import shutil
import subprocess
import sys
import tarfile

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

APP_NAME = "DICOM-Viewer-Qt6"
APP_VERSION = os.environ.get("APP_VERSION", "1.0.0")
QT_VERSION = os.environ.get("QT_VERSION", "6.12.0")

QT_DIR = ROOT / ".qt"
BUILD_DIR = ROOT / "build" / "release"
DIST_DIR = ROOT / "dist"
RELEASE_DIR = ROOT / "releases"


def run(command):
    print("\n>", subprocess.list2cmdline([str(x) for x in command])
          if os.name == "nt"
          else " ".join(str(x) for x in command))

    subprocess.run(command, check=True, cwd=ROOT)


def get_platform_tag():
    system = platform.system().lower()
    machine = platform.machine().lower()

    if system == "windows":
        return "windows-x86_64"

    if system == "linux":
        return f"linux-{machine}"

    if system == "darwin":
        return f"macos-{machine}"

    raise RuntimeError(f"Unsupported platform: {system}")


def get_qt_root():
    return next((QT_DIR / QT_VERSION).glob("*"))


def install_qt():
    print("\n========== Qt SDK ==========")

    run([
        sys.executable,
        ROOT / "scripts" / "install_qt.py"
    ])

    qt_root = get_qt_root()

    qt_config = (
        qt_root /
        "lib" /
        "cmake" /
        "Qt6" /
        "Qt6Config.cmake"
    )

    if not qt_config.exists():
        raise RuntimeError(f"Qt SDK not found: {qt_root}")

    return qt_root


def configure(qt_root):
    print("\n========== CMake Configure ==========")

    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    run([
        "cmake",
        "-S", ROOT,
        "-B", BUILD_DIR,
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_INSTALL_PREFIX={DIST_DIR}",
        f"-DQt6_ROOT={qt_root}",
        f"-DQt6_DIR={qt_root / 'lib' / 'cmake' / 'Qt6'}",
        f"-DCMAKE_PREFIX_PATH={qt_root}"
    ])


def build():
    print("\n========== Building ==========")

    run([
        "cmake",
        "--build", BUILD_DIR,
        "--config", "Release",
        "--parallel"
    ])


def deploy():
    print("\n========== Deploying ==========")

    if DIST_DIR.exists():
        shutil.rmtree(DIST_DIR)

    DIST_DIR.mkdir(parents=True)

    run([
        "cmake",
        "--install", BUILD_DIR,
        "--config", "Release",
        "--prefix", DIST_DIR.resolve()
    ])


def package():
    print("\n========== Packaging ==========")

    RELEASE_DIR.mkdir(parents=True, exist_ok=True)

    platform_tag = get_platform_tag()

    release_name = (
        f"{APP_NAME}-{APP_VERSION}-{platform_tag}"
    )

    stage = RELEASE_DIR / release_name

    if stage.exists():
        shutil.rmtree(stage)

    # Preserve symlinks, particularly important for macOS frameworks.
    shutil.copytree(
        DIST_DIR,
        stage,
        symlinks=True
    )

    system = platform.system().lower()

    if system == "windows":
        archive = RELEASE_DIR / f"{release_name}.zip"

        shutil.make_archive(
            str(archive.with_suffix("")),
            "zip",
            root_dir=RELEASE_DIR,
            base_dir=release_name
        )

    elif system == "darwin":
        archive = RELEASE_DIR / f"{release_name}.zip"

        # Preserve macOS bundle metadata and resource forks.
        run([
            "ditto",
            "-c",
            "-k",
            "--sequesterRsrc",
            "--keepParent",
            stage,
            archive
        ])

    elif system == "linux":
        archive = RELEASE_DIR / f"{release_name}.tar.gz"

        with tarfile.open(archive, "w:gz") as tar:
            tar.add(
                stage,
                arcname=release_name
            )

    else:
        raise RuntimeError("Unsupported packaging platform.")

    shutil.rmtree(stage)

    print("\n========== Release Complete ==========")
    print(f"\nArtifact: {archive.resolve()}")


def main():
    print(f"Application : {APP_NAME}")
    print(f"Version     : {APP_VERSION}")
    print(f"Qt          : {QT_VERSION}")
    print(f"Platform    : {platform.platform()}")

    qt_root = install_qt()

    configure(qt_root)
    build()
    deploy()
    package()


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"\nERROR: {error}", file=sys.stderr)
        sys.exit(1)
