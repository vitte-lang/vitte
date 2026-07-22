#!/usr/bin/env python3
"""Runtime installer contract checks that do not modify the host system."""

from __future__ import annotations

import json
import os
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / "toolchain/scripts/package/PACKAGE_VERSION").read_text(encoding="utf-8").strip()


def run(args: list[str], *, env: dict[str, str] | None = None, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd or ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )


def write_executable(path: Path, body: str) -> None:
    path.write_text(body, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def make_stub_commands(tmp: Path) -> Path:
    bin_dir = tmp / "stub-bin"
    bin_dir.mkdir(parents=True)
    stub = f"""#!/bin/sh
set -eu
case "${{1:-}}" in
  --version)
    printf '%s\\n' 'vitte {VERSION}'
    exit 0
    ;;
  --help)
    printf '%s\\n' 'Vitte command help'
    exit 0
    ;;
  check)
    [ -n "${{2:-}}" ] || {{ printf '%s\\n' 'missing input' >&2; exit 2; }}
    exit 0
    ;;
  build)
    output=
    while [ "$#" -gt 0 ]; do
      case "$1" in
        -o)
          shift
          output=${{1:-}}
          ;;
      esac
      shift || true
    done
    [ -n "$output" ] || output=a.out
    printf '%s\\n' '#!/bin/sh' 'exit 0' > "$output"
    /bin/chmod 755 "$output"
    exit 0
    ;;
esac
printf '%s\\n' 'Vitte command'
exit 0
"""
    for command in ("vitte", "vittec", "vittec0"):
        write_executable(bin_dir / command, stub)
    return bin_dir


def stage_install_prefix(tmp: Path, stub_bin: Path) -> Path:
    out = tmp / "bsd-out"
    extract = tmp / "bsd-extract"
    prefix = tmp / "installed-prefix"
    env = os.environ.copy()
    env.update(
        {
            "VERSION": VERSION,
            "OUT_DIR": str(out),
            "BSD_FAMILY": "freebsd",
            "BSD_RELEASE": "14",
            "ARCH": "amd64",
            "VITTE_BIN_AMD64": str(stub_bin / "vitte"),
            "VITTE_VITTEC_AMD64": str(stub_bin / "vittec"),
            "VITTE_VITTEC0_AMD64": str(stub_bin / "vittec0"),
        }
    )
    run([str(ROOT / "scripts_build/build-bsd-installers.sh")], env=env)
    archive = out / f"vitte-{VERSION}-freebsd-14-amd64-installer.tar.xz"
    if not archive.exists():
        raise AssertionError("BSD portable installer archive missing")
    extract.mkdir()
    subprocess.run(["tar", "-xJf", str(archive), "-C", str(extract)], check=True)
    install_env = os.environ.copy()
    install_env.update({"PREFIX": str(prefix), "PATH": os.environ.get("PATH", "/usr/bin:/bin")})
    run([str(extract / "install.sh")], env=install_env)
    return prefix


def stage_system_root(tmp: Path, stub_bin: Path) -> Path:
    dest = tmp / "system-root"
    env = os.environ.copy()
    env.update(
        {
            "VERSION": VERSION,
            "VITTE_BIN_AMD64": str(stub_bin / "vitte"),
            "VITTE_VITTEC_AMD64": str(stub_bin / "vittec"),
            "VITTE_VITTEC0_AMD64": str(stub_bin / "vittec0"),
        }
    )
    run(
        [
            str(ROOT / "scripts_build/stage-installer-payload.sh"),
            str(dest),
            "linux",
            "amd64",
            "unix",
        ],
        env=env,
    )
    return dest


def clean_env(*, path: str = "/does/not/exist") -> dict[str, str]:
    return {
        "HOME": "/nonexistent",
        "LC_ALL": "C",
        "PATH": path,
        "SHELL": "/bin/sh",
        "TERM": "dumb",
    }


def check_clean_shells(vitte: Path, work: Path) -> list[dict[str, str]]:
    shells: list[tuple[str, str | None]] = [
        ("sh", "/bin/sh"),
        ("bash", shutil.which("bash")),
        ("zsh", shutil.which("zsh")),
        ("fish", shutil.which("fish")),
    ]
    results: list[dict[str, str]] = []
    for name, shell in shells:
        if shell is None:
            results.append({"shell": name, "status": "SKIP", "reason": "not installed on host"})
            continue
        if name == "fish":
            command = f'set -gx HOME /nonexistent; set -gx LC_ALL C; set -gx PATH /does/not/exist; "{vitte}" --version'
        else:
            command = f'HOME=/nonexistent LC_ALL=C PATH=/does/not/exist "{vitte}" --version'
        completed = run([shell, "-c", command], env=clean_env(), cwd=work)
        if f"vitte {VERSION}" not in completed.stdout:
            raise AssertionError(f"{name}: unexpected --version output: {completed.stdout!r}")
        results.append({"shell": name, "status": "PASS"})
    return results


def check_absolute_path_and_dirty_path(vitte: Path, tmp: Path) -> None:
    completed = run([str(vitte), "--version"], env=clean_env())
    if completed.stdout.strip() != f"vitte {VERSION}":
        raise AssertionError(f"absolute path version mismatch: {completed.stdout!r}")

    old = tmp / "old-path"
    old.mkdir()
    write_executable(old / "vitte", "#!/bin/sh\nprintf '%s\\n' 'old vitte from PATH' >&2\nexit 99\n")
    env = clean_env(path=f"{old}:/does/not/exist")
    completed = run([str(vitte), "--version"], env=env)
    if completed.stdout.strip() != f"vitte {VERSION}":
        raise AssertionError("absolute installed wrapper was affected by old PATH")


def check_post_install_compile(vitte: Path, tmp: Path) -> None:
    work = tmp / "post-install-build"
    work.mkdir()
    source = work / "main.vit"
    source.write_text("proc main() -> int { return 0 }\n", encoding="utf-8")
    output = work / "main"
    run([str(vitte), "check", str(source)], env=clean_env(), cwd=work)
    run([str(vitte), "build", str(source), "-o", str(output)], env=clean_env(), cwd=work)
    if not output.exists():
        raise AssertionError("post-install build did not create output")
    run([str(output)], env=clean_env(), cwd=work)


def check_windows_and_macos_static_contracts() -> list[dict[str, str]]:
    checks: list[dict[str, str]] = []

    windows_cmd = ROOT / "scripts/ci/real-install-smoke.cmd"
    windows_ps1 = ROOT / "scripts/ci/real-install-smoke.ps1"
    for path in (windows_cmd, windows_ps1):
        text = path.read_text(encoding="utf-8")
        for needle in ("--version", "check", "build"):
            if needle not in text:
                raise AssertionError(f"{path.relative_to(ROOT)} missing {needle}")
        checks.append({"contract": path.relative_to(ROOT).as_posix(), "status": "PASS"})

    macos_pkg = (ROOT / "toolchain/scripts/package/make-macos-pkg.sh").read_text(encoding="utf-8")
    for needle in (".zprofile", "/usr/local/bin", "zsh", "fish"):
        if needle not in macos_pkg:
            raise AssertionError(f"macOS package script missing {needle}")
    checks.append({"contract": "macOS Terminal shell profile setup", "status": "PASS"})

    common = (ROOT / "scripts/common.sh").read_text(encoding="utf-8")
    for needle in ("glibc", "musl", "bsd-libc", "solaris-libc", "Windows XP SP3"):
        if needle not in common:
            raise AssertionError(f"scripts/common.sh missing libc/minimum contract token {needle}")
    checks.append({"contract": "libc detection and minimum-version map", "status": "PASS"})

    for arch in ("amd64", "i386", "armv6", "armv7", "aarch64", "riscv64"):
        completed = run(
            [
                "/bin/sh",
                "-c",
                f'. "{ROOT / "scripts_build/common.sh"}"; scripts_build_static_build_supported linux {arch}',
            ],
            env=clean_env(path=os.environ.get("PATH", "/usr/bin:/bin")),
        )
        if completed.returncode != 0:
            raise AssertionError(f"static build contract does not cover linux/{arch}")
    checks.append({"contract": "Linux static build architecture set", "status": "PASS"})

    return checks


def check_portable_tarball(tmp: Path, stub_bin: Path) -> dict[str, str]:
    out = tmp / "portable-out"
    env = os.environ.copy()
    env.update(
        {
            "VERSION": VERSION,
            "OUT_DIR": str(out),
            "PLATFORM": "linux",
            "ARCH": "amd64",
            "VITTE_BIN_AMD64": str(stub_bin / "vitte"),
            "VITTE_VITTEC_AMD64": str(stub_bin / "vittec"),
            "VITTE_VITTEC0_AMD64": str(stub_bin / "vittec0"),
        }
    )
    run([str(ROOT / "scripts_build/build-portable-tarball.sh")], env=env)
    archive = out / f"vitte-{VERSION}-portable-linux-amd64.tar.gz"
    manifest = Path(str(archive) + ".MANIFEST.json")
    if not archive.exists() or not manifest.exists():
        raise AssertionError("portable archive or manifest missing")
    with tarfile.open(archive, "r:gz") as tar:
        names = set(tar.getnames())
    prefix = f"vitte-{VERSION}-portable-linux-amd64"
    for required in (
        f"{prefix}/bin/vitte",
        f"{prefix}/libexec/vitte/vitte",
        f"{prefix}/share/vitte/INSTALLATION.json",
        f"{prefix}/README.portable",
    ):
        if required not in names:
            raise AssertionError(f"portable archive missing {required}")
    extract = tmp / "portable-extract"
    extract.mkdir()
    with tarfile.open(archive, "r:gz") as tar:
        tar.extractall(extract)
    vitte = extract / prefix / "bin/vitte"
    completed = run([str(vitte), "--version"], env=clean_env())
    if completed.stdout.strip() != f"vitte {VERSION}":
        raise AssertionError("portable wrapper did not run without PATH")
    data = json.loads(manifest.read_text(encoding="utf-8"))
    for key in ("version", "arch", "os", "abi", "libc", "minimum_version", "sha256", "contents", "installed_commands"):
        if key not in data:
            raise AssertionError(f"portable manifest missing {key}")
    if data["version"] != VERSION:
        raise AssertionError("portable manifest version mismatch")
    return {"contract": archive.name, "status": "PASS"}


def main() -> int:
    report_dir = ROOT / "target/reports/installers"
    report_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vitte-installer-runtime-") as raw:
        tmp = Path(raw)
        stub_bin = make_stub_commands(tmp)
        stage_system_root(tmp, stub_bin)
        installed = stage_install_prefix(tmp, stub_bin)
        vitte = installed / "bin/vitte"
        if not vitte.exists():
            raise AssertionError("installed vitte wrapper missing")

        checks: list[dict[str, str] | list[dict[str, str]]] = []
        check_absolute_path_and_dirty_path(vitte, tmp)
        checks.append({"contract": "absolute path without PATH and dirty old PATH", "status": "PASS"})
        checks.append(check_clean_shells(vitte, tmp))
        check_post_install_compile(vitte, tmp)
        checks.append({"contract": "post-install check/build/run", "status": "PASS"})
        checks.extend(check_windows_and_macos_static_contracts())
        checks.append(check_portable_tarball(tmp, stub_bin))

        flattened: list[dict[str, str]] = []
        for item in checks:
            if isinstance(item, list):
                flattened.extend(item)
            else:
                flattened.append(item)
        report = {
            "schema": "org.vitte.installer-runtime-contract.v1",
            "version": VERSION,
            "status": "PASS",
            "checks": flattened,
        }
        (report_dir / "installer-runtime-contract.json").write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    print("[installer-runtime-contract] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
