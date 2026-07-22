#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
PERL_ROOT="$ROOT_DIR/src/vitte/packages/perl"
INDEX="$PERL_ROOT/packages.json"
OUT_ROOT="$ROOT_DIR/target/perl-packages"
ARCHIVE_DIR="$OUT_ROOT/archives"
INSTALL_ROOT="$OUT_ROOT/install-test"
REPORT_DIR="$ROOT_DIR/target/reports"
REPORT_JSON="$REPORT_DIR/perl_packages_check.json"
REPORT_MD="$REPORT_DIR/perl_packages_check.md"
REGISTRY_JSON="$OUT_ROOT/registry.json"
CHECKSUMS="$OUT_ROOT/checksums.sha256"
VERSION_REQUIRED="0.1.0"

mkdir -p "$ARCHIVE_DIR" "$INSTALL_ROOT" "$REPORT_DIR"
: > "$CHECKSUMS"

die() {
  printf '[perl-packages-check][error] %s\n' "$*" >&2
  exit 1
}

[ -d "$PERL_ROOT" ] || die "missing Perl packages root: $PERL_ROOT"
[ -f "$INDEX" ] || die "missing index: $INDEX"
command -v perl >/dev/null 2>&1 || die "perl is required"
command -v python3 >/dev/null 2>&1 || die "python3 is required"

python3 - "$ROOT_DIR" "$INDEX" "$OUT_ROOT" "$ARCHIVE_DIR" "$INSTALL_ROOT" "$REGISTRY_JSON" "$CHECKSUMS" "$REPORT_JSON" "$REPORT_MD" "$VERSION_REQUIRED" <<'PY'
from __future__ import annotations

import gzip
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


ROOT = Path(sys.argv[1])
INDEX = Path(sys.argv[2])
OUT_ROOT = Path(sys.argv[3])
ARCHIVE_DIR = Path(sys.argv[4])
INSTALL_ROOT = Path(sys.argv[5])
REGISTRY_JSON = Path(sys.argv[6])
CHECKSUMS = Path(sys.argv[7])
REPORT_JSON = Path(sys.argv[8])
REPORT_MD = Path(sys.argv[9])
VERSION_REQUIRED = sys.argv[10]
PERL_ROOT = INDEX.parent


def fail(failures: list[str], message: str) -> None:
    failures.append(message)


def run(cmd: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> tuple[bool, str]:
    proc = subprocess.run(
        cmd,
        cwd=cwd or ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return proc.returncode == 0, proc.stdout.strip()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def stable_tar_gz(source_dir: Path, archive_path: Path, prefix: str) -> None:
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(delete=False) as raw_tmp:
        raw_path = Path(raw_tmp.name)
    try:
        with tarfile.open(raw_path, "w", format=tarfile.PAX_FORMAT) as tar:
            for path in sorted(source_dir.rglob("*")):
                rel = path.relative_to(source_dir).as_posix()
                arcname = f"{prefix}/{rel}"
                info = tar.gettarinfo(str(path), arcname=arcname)
                info.uid = 0
                info.gid = 0
                info.uname = ""
                info.gname = ""
                info.mtime = 0
                if path.is_dir():
                    info.mode = 0o755
                    tar.addfile(info)
                else:
                    info.mode = 0o644
                    with path.open("rb") as handle:
                        tar.addfile(info, handle)
        with raw_path.open("rb") as src, gzip.GzipFile(
            filename="",
            mode="wb",
            fileobj=archive_path.open("wb"),
            mtime=0,
        ) as gz:
            shutil.copyfileobj(src, gz)
    finally:
        raw_path.unlink(missing_ok=True)


def extract_archive(archive: Path, dest: Path) -> None:
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True)
    with tarfile.open(archive, "r:gz") as tar:
        tar.extractall(dest, filter="data")


def export_ok_symbols(module_file: Path) -> list[str]:
    text = module_file.read_text(encoding="utf-8")
    match = re.search(r"our\s+\@EXPORT_OK\s*=\s*qw\((.*?)\);", text, re.S)
    if not match:
        return []
    return match.group(1).split()


def validate_manifest(pkgdir: Path, failures: list[str], name: str) -> None:
    manifest = pkgdir / "MANIFEST"
    if not manifest.exists():
        fail(failures, f"{name}: missing MANIFEST")
        return
    listed = {
        line.strip()
        for line in manifest.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    }
    for path in sorted(pkgdir.rglob("*")):
        if path.is_dir():
            continue
        rel = path.relative_to(pkgdir).as_posix()
        if rel == "MANIFEST":
            continue
        if rel not in listed:
            fail(failures, f"{name}: MANIFEST missing {rel}")


def validate_quality_gate(pkgdir: Path, package: dict[str, object], declared_exports: list[str], failures: list[str], name: str) -> None:
    quality_gate = package.get("quality_gate", {})
    if not isinstance(quality_gate, dict):
        return
    minimum_exports = quality_gate.get("minimum_exports")
    if isinstance(minimum_exports, int) and len(declared_exports) < minimum_exports:
        fail(failures, f"{name}: expected at least {minimum_exports} exports, found {len(declared_exports)}")

    required = [
        "LICENSE",
        "Changes",
        "MANIFEST",
        "MANIFEST.SKIP",
        "Makefile.PL",
        "cpanfile",
        "docs/api.md",
        "docs/design.md",
        "docs/error-codes.md",
        "docs/compatibility.md",
    ]
    if quality_gate.get("manifest_required"):
        for rel in required:
            if not (pkgdir / rel).exists():
                fail(failures, f"{name}: missing required distribution file {rel}")
        validate_manifest(pkgdir, failures, name)


def validate() -> int:
    failures: list[str] = []
    index = json.loads(INDEX.read_text(encoding="utf-8"))
    packages = index.get("packages", [])

    if index.get("schema") != "vitte.perl-packages.index":
        fail(failures, "packages.json schema must be vitte.perl-packages.index")
    if index.get("version") != VERSION_REQUIRED:
        fail(failures, f"packages.json version must be {VERSION_REQUIRED}")
    if index.get("package_count") != len(packages):
        fail(failures, "packages.json package_count does not match packages length")
    if len(packages) != 20:
        fail(failures, f"expected 20 packages, found {len(packages)}")

    seen: set[str] = set()
    registry_packages: list[dict[str, object]] = []
    report_packages: list[dict[str, object]] = []
    checksum_lines: list[str] = []

    for package in packages:
        name = package.get("name")
        if not isinstance(name, str) or not name:
            fail(failures, "package entry missing name")
            continue
        if name in seen:
            fail(failures, f"duplicate package {name}")
        seen.add(name)

        pkgdir = PERL_ROOT / name
        meta_path = pkgdir / str(package.get("meta", "META.json"))
        readme_path = pkgdir / str(package.get("readme", "README.md"))
        owners_path = pkgdir / str(package.get("owners", "OWNERS"))
        test_path = pkgdir / str(package.get("test", "t/basic.t"))
        module_file = pkgdir / str(package.get("module_file", ""))
        module_name = package.get("module")
        dist = package.get("distribution")
        declared_exports = package.get("exports", [])

        for label, path in (
            ("directory", pkgdir),
            ("META.json", meta_path),
            ("README.md", readme_path),
            ("OWNERS", owners_path),
            ("module .pm", module_file),
            ("t/basic.t", test_path),
        ):
            if not path.exists():
                fail(failures, f"{name}: missing {label}: {path.relative_to(ROOT)}")

        if not meta_path.exists():
            continue
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        if meta.get("version") != VERSION_REQUIRED:
            fail(failures, f"{name}: META version must be {VERSION_REQUIRED}")
        if meta.get("name") != dist:
            fail(failures, f"{name}: META name differs from packages.json distribution")
        if meta.get("x_vitte_package") != f"perl/{name}":
            fail(failures, f"{name}: META x_vitte_package must be perl/{name}")
        prereqs = meta.get("prereqs", {})
        runtime_requires = prereqs.get("runtime", {}).get("requires", {}) if isinstance(prereqs, dict) else {}
        test_requires = prereqs.get("test", {}).get("requires", {}) if isinstance(prereqs, dict) else {}
        for dep in ("perl", "Exporter"):
            if dep not in runtime_requires:
                fail(failures, f"{name}: META prereqs.runtime.requires missing {dep}")
        for dep in ("Test::More", "FindBin"):
            if dep not in test_requires:
                fail(failures, f"{name}: META prereqs.test.requires missing {dep}")

        provides = meta.get("provides", {})
        if module_name not in provides:
            fail(failures, f"{name}: META provides missing {module_name}")
        else:
            provided = provides[module_name]
            if provided.get("file") != package.get("module_file"):
                fail(failures, f"{name}: provided module file mismatch")
            if provided.get("version") != VERSION_REQUIRED:
                fail(failures, f"{name}: provided module version must be {VERSION_REQUIRED}")

        if module_file.exists():
            module_exports = export_ok_symbols(module_file)
            if sorted(module_exports) != sorted(declared_exports):
                fail(failures, f"{name}: packages.json exports differ from module @EXPORT_OK")
            if readme_path.exists():
                readme = readme_path.read_text(encoding="utf-8")
                for symbol in declared_exports:
                    if f"`{symbol}`" not in readme:
                        fail(failures, f"{name}: README.md does not document export {symbol}")
            if test_path.exists():
                test_text = test_path.read_text(encoding="utf-8")
                for symbol in declared_exports:
                    if symbol not in test_text:
                        fail(failures, f"{name}: {test_path.relative_to(pkgdir)} does not mention export {symbol}")

            env = os.environ.copy()
            env["PERL5LIB"] = str(pkgdir / "lib")
            ok, output = run(["perl", "-c", str(module_file)], env=env)
            if not ok:
                fail(failures, f"{name}: perl -c failed: {output}")
            for pm_file in sorted((pkgdir / "lib").rglob("*.pm")):
                ok, output = run(["perl", "-c", str(pm_file)], env=env)
                if not ok:
                    fail(failures, f"{name}: perl -c failed for {pm_file.relative_to(pkgdir)}: {output}")
            version_check = (
                f"use lib q({pkgdir / 'lib'}); "
                f"use {module_name}; "
                f"no strict q(refs); "
                f"die ${{q({module_name}::VERSION)}} unless ${{q({module_name}::VERSION)}} eq q({VERSION_REQUIRED});"
            )
            ok, output = run(["perl", "-e", version_check])
            if not ok:
                fail(failures, f"{name}: PERL5LIB module load failed: {output}")

        if test_path.exists():
            env = os.environ.copy()
            env["PERL5LIB"] = str(pkgdir / "lib")
            for test_file in sorted((pkgdir / "t").glob("*.t")):
                ok, output = run(["perl", str(test_file)], env=env)
                if not ok:
                    fail(failures, f"{name}: {test_file.relative_to(pkgdir)} failed: {output}")

        examples_dir = pkgdir / "examples"
        if examples_dir.exists():
            env = os.environ.copy()
            env["PERL5LIB"] = str(pkgdir / "lib")
            for example in sorted(examples_dir.glob("*.pl")):
                ok, output = run(["perl", "-c", str(example)], env=env)
                if not ok:
                    fail(failures, f"{name}: example compile failed for {example.relative_to(pkgdir)}: {output}")

        xt_dir = pkgdir / "xt"
        if xt_dir.exists():
            env = os.environ.copy()
            env["PERL5LIB"] = str(pkgdir / "lib")
            for xt_test in sorted(xt_dir.glob("*.t")):
                ok, output = run(["perl", str(xt_test.relative_to(pkgdir))], cwd=pkgdir, env=env)
                if not ok:
                    fail(failures, f"{name}: {xt_test.relative_to(pkgdir)} failed: {output}")

        validate_quality_gate(pkgdir, package, declared_exports, failures, name)

        archive_name = f"{dist}-{VERSION_REQUIRED}.tar.gz"
        archive_path = ARCHIVE_DIR / archive_name
        stable_tar_gz(pkgdir, archive_path, f"{dist}-{VERSION_REQUIRED}")
        digest = sha256(archive_path)
        checksum_lines.append(f"{digest}  archives/{archive_name}")

        install_dest = INSTALL_ROOT / name
        extract_archive(archive_path, install_dest)
        extracted_root = install_dest / f"{dist}-{VERSION_REQUIRED}"
        env = os.environ.copy()
        env["PERL5LIB"] = str(extracted_root / "lib")
        for extracted_test in sorted((extracted_root / "t").glob("*.t")):
            ok, output = run(["perl", str(extracted_test)], env=env)
            if not ok:
                fail(failures, f"{name}: extracted install test failed for {extracted_test.relative_to(extracted_root)}: {output}")

        registry_packages.append({
            "name": name,
            "distribution": dist,
            "version": VERSION_REQUIRED,
            "module": module_name,
            "archive": f"archives/{archive_name}",
            "sha256": digest,
            "dependencies": prereqs,
        })
        report_packages.append({
            "name": name,
            "distribution": dist,
            "module": module_name,
            "archive": f"archives/{archive_name}",
            "sha256": digest,
        })

    CHECKSUMS.write_text("\n".join(checksum_lines) + ("\n" if checksum_lines else ""), encoding="utf-8")
    registry = {
        "schema": "vitte.perl-packages.registry",
        "schema_version": "1.0.0",
        "version": VERSION_REQUIRED,
        "package_count": len(registry_packages),
        "packages": registry_packages,
    }
    REGISTRY_JSON.write_text(json.dumps(registry, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    payload = {
        "schema": "vitte.perl-packages.check-report",
        "schema_version": "1.0.0",
        "version": VERSION_REQUIRED,
        "status": "fail" if failures else "ok",
        "package_count": len(packages),
        "archive_count": len(registry_packages),
        "registry": str(REGISTRY_JSON.relative_to(ROOT)),
        "checksums": str(CHECKSUMS.relative_to(ROOT)),
        "packages": report_packages,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    md = [
        "# Perl Packages Check",
        "",
        f"- status: `{payload['status']}`",
        f"- version: `{VERSION_REQUIRED}`",
        f"- packages: `{len(packages)}`",
        f"- archives: `{len(registry_packages)}`",
        f"- registry: `{payload['registry']}`",
        f"- checksums: `{payload['checksums']}`",
        "",
        "## Packages",
        "",
    ]
    for package in report_packages:
        md.append(f"- `{package['distribution']}` `{package['module']}` `{package['sha256']}`")
    if failures:
        md.extend(["", "## Failures", ""])
        for failure in failures:
            md.append(f"- {failure}")
    REPORT_MD.write_text("\n".join(md).rstrip() + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[perl-packages-check][error] {failure}", file=sys.stderr)
        return 1

    print(
        "[perl-packages-check] OK "
        f"packages={len(packages)} archives={len(registry_packages)} "
        f"registry={REGISTRY_JSON.relative_to(ROOT)}"
    )
    return 0


raise SystemExit(validate())
PY
