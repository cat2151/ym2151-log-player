#!/usr/bin/env python3
"""
Build script for ym2151-zig-cc test program
Supports Linux, Windows (native), and cross-compilation
"""

import os
import platform
import subprocess
import sys
from pathlib import Path


def run_command(cmd, description):
    """Run a shell command and handle errors."""
    print(f">>> {description}")
    print(f"    {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Error: {description} failed")
        if e.stdout:
            print(e.stdout)
        if e.stderr:
            print(e.stderr)
        return False
    except FileNotFoundError:
        print(f"❌ Error: Command not found: {cmd[0]}")
        print("Please ensure zig is installed and in your PATH")
        return False


def check_zig():
    """Check if zig is available."""
    try:
        result = subprocess.run(["zig", "version"], capture_output=True, text=True)
        version = result.stdout.strip()
        print(f"✓ Zig version: {version}")
        return True
    except FileNotFoundError:
        print("❌ Error: zig not found in PATH")
        print()
        print("Please install zig:")
        print("  Linux:   https://ziglang.org/download/")
        print("  Windows: https://ziglang.org/download/")
        print("  Or see BUILD.md for installation instructions")
        return False


def build_linux(use_zig=True):
    """Build for Linux."""
    print("\n" + "=" * 60)
    print("Building for Linux")
    print("=" * 60)

    if use_zig:
        if not check_zig():
            return False

        cmd = ["zig", "cc", "-o", "test_opm", "src/phase1/test_opm.c", "opm.c", "-lm", "-fwrapv"]
        if not run_command(cmd, "Building with zig cc"):
            return False
    else:
        cmd = ["gcc", "-o", "test_opm", "src/phase1/test_opm.c", "opm.c", "-lm", "-fwrapv"]
        if not run_command(cmd, "Building with gcc"):
            return False

    print("✅ Build successful: test_opm")
    return True


def build_windows(cross_compile=False):
    """Build for Windows."""
    print("\n" + "=" * 60)
    if cross_compile:
        print("Cross-compiling for Windows (from Linux)")
    else:
        print("Building for Windows")
    print("=" * 60)

    if not check_zig():
        return False

    if cross_compile:
        cmd = [
            "zig",
            "cc",
            "-target",
            "x86_64-windows",
            "-o",
            "test_opm.exe",
            "src/phase1/test_opm.c",
            "opm.c",
            "-lm",
            "-fwrapv",
        ]
    else:
        cmd = ["zig", "cc", "-o", "test_opm.exe", "src/phase1/test_opm.c", "opm.c", "-lm", "-fwrapv"]

    if not run_command(cmd, "Building with zig cc"):
        return False

    print("✅ Build successful: test_opm.exe")
    return True


def build_phase2_windows(cross_compile=False):
    """Build phase2 WAV output program for Windows."""
    print("\n" + "=" * 60)
    if cross_compile:
        print("Cross-compiling phase2 WAV output for Windows (from Linux)")
    else:
        print("Building phase2 WAV output for Windows")
    print("=" * 60)

    if not check_zig():
        return False

    if cross_compile:
        cmd = [
            "zig",
            "cc",
            "-target",
            "x86_64-windows",
            "-o",
            "wav_output.exe",
            "src/phase2/wav_output.c",
            "opm.c",
            "-lm",
            "-fwrapv",
        ]
    else:
        cmd = ["zig", "cc", "-o", "wav_output.exe", "src/phase2/wav_output.c", "opm.c", "-lm", "-fwrapv"]

    if not run_command(cmd, "Building phase2 WAV output with zig cc"):
        return False

    print("✅ Build successful: wav_output.exe")
    return True


def build_phase2_linux(use_zig=True):
    """Build phase2 WAV output program for Linux."""
    print("\n" + "=" * 60)
    print("Building phase2 WAV output for Linux")
    print("=" * 60)

    if use_zig:
        if not check_zig():
            return False

        cmd = ["zig", "cc", "-o", "wav_output", "src/phase2/wav_output.c", "opm.c", "-lm", "-fwrapv"]
        if not run_command(cmd, "Building phase2 WAV output with zig cc"):
            return False
    else:
        cmd = ["gcc", "-o", "wav_output", "src/phase2/wav_output.c", "opm.c", "-lm", "-fwrapv"]
        if not run_command(cmd, "Building phase2 WAV output with gcc"):
            return False

    print("✅ Build successful: wav_output")
    return True


def build_phase3_windows(cross_compile=False):
    """Build phase3 real-time audio program for Windows."""
    print("\n" + "=" * 60)
    if cross_compile:
        print("Cross-compiling phase3 real-time audio for Windows (from Linux)")
    else:
        print("Building phase3 real-time audio for Windows")
    print("=" * 60)

    if not check_zig():
        return False

    if cross_compile:
        cmd = [
            "zig",
            "cc",
            "-target",
            "x86_64-windows",
            "-o",
            "real_time_audio.exe",
            "src/phase3/real_time_audio.c",
            "opm.c",
            "-lm",
            "-fwrapv",
            "-O3",
        ]
    else:
        cmd = [
            "zig",
            "cc",
            "-o",
            "real_time_audio.exe",
            "src/phase3/real_time_audio.c",
            "opm.c",
            "-lm",
            "-fwrapv",
            "-O3",
        ]

    if not run_command(cmd, "Building phase3 real-time audio with zig cc"):
        return False

    print("✅ Build successful: real_time_audio.exe")
    return True


def build_phase3_linux(use_zig=True):
    """Build phase3 real-time audio program for Linux."""
    print("\n" + "=" * 60)
    print("Building phase3 real-time audio for Linux")
    print("=" * 60)

    if use_zig:
        if not check_zig():
            return False

        cmd = [
            "zig",
            "cc",
            "-o",
            "real_time_audio",
            "src/phase3/real_time_audio.c",
            "opm.c",
            "-lm",
            "-lpthread",
            "-ldl",
            "-fwrapv",
        ]
        if not run_command(cmd, "Building phase3 real-time audio with zig cc"):
            return False
    else:
        cmd = [
            "gcc",
            "-o",
            "real_time_audio",
            "src/phase3/real_time_audio.c",
            "opm.c",
            "-lm",
            "-lpthread",
            "-ldl",
            "-fwrapv",
        ]
        if not run_command(cmd, "Building phase3 real-time audio with gcc"):
            return False

    print("✅ Build successful: real_time_audio")
    return True


def build_phase4_windows(cross_compile=False):
    """Build YM2151 log player for Windows."""
    print("\n" + "=" * 60)
    if cross_compile:
        print("Cross-compiling YM2151 log player for Windows (from Linux)")
    else:
        print("Building YM2151 log player for Windows")
    print("=" * 60)

    if not check_zig():
        return False

    if cross_compile:
        cmd = [
            "zig",
            "cc",
            "-target",
            "x86_64-windows",
            "-o",
            "player.exe",
            "src/player.c",
            "opm.c",
            "-lm",
            "-fwrapv",
            "-O3",
        ]
    else:
        cmd = ["zig", "cc", "-o", "player.exe", "src/player.c", "opm.c", "-lm", "-fwrapv", "-O3"]

    if not run_command(cmd, "Building YM2151 log player with zig cc"):
        return False

    print("✅ Build successful: player.exe")
    return True


def build_phase4_linux(use_zig=True):
    """Build YM2151 log player for Linux."""
    print("\n" + "=" * 60)
    print("Building YM2151 log player for Linux")
    print("=" * 60)

    if use_zig:
        if not check_zig():
            return False

        cmd = [
            "zig",
            "cc",
            "-o",
            "player",
            "src/player.c",
            "opm.c",
            "-lm",
            "-lpthread",
            "-ldl",
            "-fwrapv",
        ]
        if not run_command(cmd, "Building YM2151 log player with zig cc"):
            return False
    else:
        cmd = [
            "gcc",
            "-o",
            "player",
            "src/player.c",
            "opm.c",
            "-lm",
            "-lpthread",
            "-ldl",
            "-fwrapv",
        ]
        if not run_command(cmd, "Building YM2151 log player with gcc"):
            return False

    print("✅ Build successful: player")
    return True


def run_test():
    """Run the test program."""
    print("\n" + "=" * 60)
    print("Running test")
    print("=" * 60)

    system = platform.system()
    if system == "Windows":
        executable = "test_opm.exe"
    else:
        executable = "./test_opm"

    if not Path(executable if system != "Windows" else "test_opm.exe").exists():
        print(f"❌ Error: {executable} not found")
        print("Please build first")
        return False

    try:
        result = subprocess.run([executable], check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Test failed with exit code {e.returncode}")
        return False


def main():
    """Main entry point."""
    print("YM2151 Log Player Build Script")
    print("=" * 60)

    # Parse command line arguments
    if len(sys.argv) > 1:
        command = sys.argv[1].lower()
    else:
        command = "build"

    system = platform.system()

    success = False

    if command == "build":
        if system == "Windows":
            success = build_windows(cross_compile=False)
        else:
            success = build_linux(use_zig=True)

        if success:
            run_test()

    elif command == "build-gcc":
        if system != "Linux":
            print("❌ Error: gcc build only supported on Linux")
            return 1
        success = build_linux(use_zig=False)
        if success:
            run_test()

    elif command == "build-windows":
        success = build_windows(cross_compile=(system != "Windows"))

    elif command == "build-phase2":
        if system == "Windows":
            success = build_phase2_windows(cross_compile=False)
        else:
            success = build_phase2_linux(use_zig=True)

    elif command == "build-phase2-gcc":
        if system != "Linux":
            print("❌ Error: gcc build only supported on Linux")
            return 1
        success = build_phase2_linux(use_zig=False)

    elif command == "build-phase2-windows":
        success = build_phase2_windows(cross_compile=(system != "Windows"))

    elif command == "build-phase3":
        if system == "Windows":
            success = build_phase3_windows(cross_compile=False)
        else:
            success = build_phase3_linux(use_zig=True)

    elif command == "build-phase3-gcc":
        if system != "Linux":
            print("❌ Error: gcc build only supported on Linux")
            return 1
        success = build_phase3_linux(use_zig=False)

    elif command == "build-phase3-windows":
        success = build_phase3_windows(cross_compile=(system != "Windows"))

    elif command == "build-phase4":
        if system == "Windows":
            success = build_phase4_windows(cross_compile=False)
        else:
            success = build_phase4_linux(use_zig=True)

    elif command == "build-phase4-gcc":
        if system != "Linux":
            print("❌ Error: gcc build only supported on Linux")
            return 1
        success = build_phase4_linux(use_zig=False)

    elif command == "build-phase4-windows":
        success = build_phase4_windows(cross_compile=(system != "Windows"))

    elif command == "test":
        success = run_test()

    elif command == "help":
        print("Usage: python3 build.py [command]")
        print()
        print("Commands:")
        print("  build-phase4         Build YM2151 log player for current platform (recommended)")
        print("  build-phase4-gcc     Build YM2151 log player with gcc (Linux only)")
        print("  build-phase4-windows Build YM2151 log player Windows executable (cross-compile if on Linux)")
        print("  help                 Show this help message")
        print()
        print("Note: Other build commands from ym2151-zig-cc are preserved but not used in this project.")
        return 0

    else:
        print(f"❌ Unknown command: {command}")
        print("Run 'python3 build.py help' for usage")
        return 1

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
