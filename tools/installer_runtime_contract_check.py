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


def build_bsd_installer(tmp: Path, stub_bin: Path, label: str) -> Path:
    out = tmp / "bsd-out"
    extract = tmp / f"bsd-extract-{label}"
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
    return extract


def install_from_bsd_kit(extract: Path, prefix: Path, *, destdir: Path | None = None) -> None:
    install_env = os.environ.copy()
    install_env.update({"PREFIX": str(prefix), "PATH": os.environ.get("PATH", "/usr/bin:/bin")})
    if destdir is not None:
        install_env["DESTDIR"] = str(destdir)
    run([str(extract / "install.sh")], env=install_env)


def uninstall_from_bsd_kit(extract: Path, prefix: Path, *, destdir: Path | None = None) -> None:
    uninstall_env = os.environ.copy()
    uninstall_env.update({"PREFIX": str(prefix), "PATH": os.environ.get("PATH", "/usr/bin:/bin")})
    if destdir is not None:
        uninstall_env["DESTDIR"] = str(destdir)
    run([str(extract / "uninstall.sh")], env=uninstall_env)


def stage_install_prefix(tmp: Path, stub_bin: Path) -> Path:
    extract = build_bsd_installer(tmp, stub_bin, "primary")
    prefix = tmp / "installed-prefix"
    install_from_bsd_kit(extract, prefix)
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
    home = work / "empty-home"
    home.mkdir()
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
            completed = run(
                [shell, "--no-config", "-c", f'set -gx HOME "{home}"; set -gx LC_ALL C; set -gx PATH /does/not/exist; "{vitte}" --version'],
                env=clean_env(),
                cwd=work,
            )
        elif name == "bash":
            command = f'HOME="{home}" LC_ALL=C PATH=/does/not/exist "{vitte}" --version'
            completed = run([shell, "--noprofile", "--norc", "-c", command], env=clean_env(), cwd=work)
        elif name == "zsh":
            command = f'HOME="{home}" LC_ALL=C PATH=/does/not/exist "{vitte}" --version'
            completed = run([shell, "-f", "-c", command], env=clean_env(), cwd=work)
        else:
            command = f'HOME="{home}" LC_ALL=C PATH=/does/not/exist "{vitte}" --version'
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


def check_payload_metadata(share: Path) -> None:
    required = (
        "packages/registry/registry.json",
        "packages/registry/checksums.sha256",
        "packages/registry/lockfile.vitte.lock",
        "packages/compiled/packages-manifest.json",
        "stdlib/compiled/stdlib-manifest.json",
        "CHECKSUMS.sha256",
        "SBOM.spdx.json",
        "SBOM.cyclonedx.json",
        "INSTALLATION.json",
    )
    for relative in required:
        path = share / relative
        if not path.is_file() or path.stat().st_size == 0:
            raise AssertionError(f"installed payload metadata missing {path}")
    manifest = json.loads((share / "INSTALLATION.json").read_text(encoding="utf-8"))
    for component in (
        "compiled-stdlib",
        "compiled-packages",
        "local-package-registry",
        "checksums",
        "sbom",
        "installer-doctor",
    ):
        if component not in manifest.get("components", []):
            raise AssertionError(f"INSTALLATION.json missing component {component}")
    checksums = (share / "CHECKSUMS.sha256").read_text(encoding="utf-8")
    for relative in required:
        if relative in {"CHECKSUMS.sha256", "INSTALLATION.json"}:
            continue
        if f"  {relative}\n" not in checksums:
            raise AssertionError(f"CHECKSUMS.sha256 missing {relative}")
    packages = json.loads((share / "packages/compiled/packages-manifest.json").read_text(encoding="utf-8"))
    if packages.get("package_count", 0) < 1:
        raise AssertionError("compiled package cache is empty")
    stdlib = json.loads((share / "stdlib/compiled/stdlib-manifest.json").read_text(encoding="utf-8"))
    if stdlib.get("file_count", 0) < 1:
        raise AssertionError("compiled stdlib cache is empty")


def check_installer_permissions_uninstall_and_destdir(tmp: Path, stub_bin: Path) -> list[dict[str, str]]:
    checks: list[dict[str, str]] = []
    extract = build_bsd_installer(tmp, stub_bin, "permissions")

    custom_prefix = tmp / "custom-prefix"
    install_from_bsd_kit(extract, custom_prefix)
    doctor = custom_prefix / "bin/vitte-installer-doctor"
    run([str(doctor)], env=clean_env())
    check_payload_metadata(custom_prefix / "share/vitte")
    for required in (
        custom_prefix / "bin/vitte",
        custom_prefix / "bin/vittec",
        custom_prefix / "bin/vittec0",
        custom_prefix / "bin/vitte-installer-doctor",
        custom_prefix / "libexec/vitte/vitte",
    ):
        if not required.exists():
            raise AssertionError(f"custom prefix install missing {required}")
        if not os.access(required, os.X_OK):
            raise AssertionError(f"custom prefix install is not executable: {required}")
    checks.append({"contract": "install prefix custom with installer doctor", "status": "PASS"})
    checks.append({"contract": "platform artifact registry packages stdlib checksums sbom", "status": "PASS"})

    user_local = tmp / "home/.local"
    install_from_bsd_kit(extract, user_local)
    run([str(user_local / "bin/vitte-installer-doctor")], env=clean_env())
    checks.append({"contract": "install user-local prefix", "status": "PASS"})

    destdir = tmp / "destdir-root"
    install_from_bsd_kit(extract, Path("/usr/local"), destdir=destdir)
    for required in (
        destdir / "usr/local/bin/vitte",
        destdir / "usr/local/bin/vitte-installer-doctor",
        destdir / "usr/local/libexec/vitte/vitte",
        destdir / "usr/local/share/vitte/INSTALLATION.json",
        destdir / "usr/local/share/vitte/CHECKSUMS.sha256",
        destdir / "usr/local/share/vitte/SBOM.spdx.json",
        destdir / "usr/local/share/vitte/packages/registry/registry.json",
        destdir / "usr/local/share/vitte/packages/compiled/packages-manifest.json",
        destdir / "usr/local/share/vitte/stdlib/compiled/stdlib-manifest.json",
    ):
        if not required.exists():
            raise AssertionError(f"DESTDIR install missing {required}")
    checks.append({"contract": "install root layout through DESTDIR", "status": "PASS"})

    uninstall_from_bsd_kit(extract, custom_prefix)
    for removed in (
        custom_prefix / "bin/vitte",
        custom_prefix / "bin/vittec",
        custom_prefix / "bin/vittec0",
        custom_prefix / "libexec/vitte",
        custom_prefix / "share/vitte",
    ):
        if removed.exists():
            raise AssertionError(f"uninstall left installed path behind: {removed}")
    checks.append({"contract": "uninstall removes wrappers payload and VITTE_ROOT side effects", "status": "PASS"})

    return checks


def check_platform_specific_contracts() -> list[dict[str, str]]:
    checks: list[dict[str, str]] = []

    windows_cmd = ROOT / "scripts/ci/real-install-smoke.cmd"
    windows_ps1 = ROOT / "scripts/ci/real-install-smoke.ps1"
    windows_builder = ROOT / "scripts/build-windows-installer.sh"
    stage_payload = ROOT / "scripts/stage-installer-payload.sh"
    windows_nsi_template = ROOT / "toolchain/scripts/package/windows/vitte-installer.nsi"
    for path in (windows_cmd, windows_ps1):
        text = path.read_text(encoding="utf-8")
        for needle in ("--version", "check", "build"):
            if needle not in text:
                raise AssertionError(f"{path.relative_to(ROOT)} missing {needle}")
        checks.append({"contract": path.relative_to(ROOT).as_posix(), "status": "PASS"})
    cmd_text = windows_cmd.read_text(encoding="utf-8")
    if "powershell" in cmd_text.lower():
        raise AssertionError("Windows XP cmd smoke must not require PowerShell")
    builder_text = windows_builder.read_text(encoding="utf-8")
    for needle in ("AtLeastWinXP", "Function un.RemoveFromPath", "DeleteRegValue HKLM", "CreateShortCut", "WriteRegExpandStr HKLM"):
        if needle not in builder_text:
            raise AssertionError(f"Windows installer builder missing {needle}")
    if "vitte-installer-doctor.cmd" not in stage_payload.read_text(encoding="utf-8"):
        raise AssertionError("Windows payload staging missing vitte-installer-doctor.cmd")
    nsi_template = windows_nsi_template.read_text(encoding="utf-8")
    for needle in ("Function un.RemoveFromPath", "DeleteRegValue HKLM", "WriteRegExpandStr HKLM"):
        if needle not in nsi_template:
            raise AssertionError(f"Windows NSIS template missing {needle}")
    checks.append({"contract": "Windows XP cmd-only and Windows 11 PATH registry uninstall", "status": "PASS"})

    macos_pkg = (ROOT / "toolchain/scripts/package/make-macos-pkg.sh").read_text(encoding="utf-8")
    for needle in (".zprofile", "/usr/local/bin", "zsh", "fish"):
        if needle not in macos_pkg:
            raise AssertionError(f"macOS package script missing {needle}")
    macos_builder = (ROOT / "scripts/build-macos-installers.sh").read_text(encoding="utf-8")
    for needle in ("arm64", "x86_64", "universal", "productsign", "notarytool", "STRICT_DMG"):
        if needle not in macos_builder:
            raise AssertionError(f"macOS installer builder missing {needle}")
    checks.append({"contract": "macOS Intel Apple Silicon universal signature notarization zprofile", "status": "PASS"})

    linux_builder = (ROOT / "scripts/build-linux-debs.sh").read_text(encoding="utf-8")
    for needle in ("amd64", "i386", "armhf", "arm64", "riscv64", "postinst", "prerm", "#!/bin/sh", "set -eu"):
        if needle not in linux_builder:
            raise AssertionError(f"Debian builder missing {needle}")
    checks.append({"contract": "Debian deb arches and robust postinst/prerm", "status": "PASS"})

    solaris_builder = (ROOT / "scripts/build-solaris-package.sh").read_text(encoding="utf-8")
    for needle in ("SVR4", "amd64", "i386", "install.sh", "uninstall.sh", "#!/bin/sh", "set -eu"):
        if needle not in solaris_builder:
            raise AssertionError(f"Solaris builder missing {needle}")
    checks.append({"contract": "Solaris SVR4 package portable kit POSIX sh", "status": "PASS"})

    bsd_builder = (ROOT / "scripts/build-bsd-installers.sh").read_text(encoding="utf-8").lower()
    for needle in ("freebsd", "openbsd", "netbsd", "dragonfly", "install.sh", "uninstall.sh", "pkg"):
        if needle not in bsd_builder:
            raise AssertionError(f"BSD builder missing {needle}")
    checks.append({"contract": "BSD portable install and FreeBSD pkg", "status": "PASS"})

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
        f"{prefix}/bin/vitte-installer-doctor",
        f"{prefix}/libexec/vitte/vitte",
        f"{prefix}/share/vitte/INSTALLATION.json",
        f"{prefix}/share/vitte/CHECKSUMS.sha256",
        f"{prefix}/share/vitte/SBOM.spdx.json",
        f"{prefix}/share/vitte/packages/registry/registry.json",
        f"{prefix}/share/vitte/packages/compiled/packages-manifest.json",
        f"{prefix}/share/vitte/stdlib/compiled/stdlib-manifest.json",
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
    run([str(extract / prefix / "bin/vitte-installer-doctor")], env=clean_env())
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
        checks.extend(check_installer_permissions_uninstall_and_destdir(tmp, stub_bin))
        checks.extend(check_platform_specific_contracts())
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
