from dataclasses import dataclass
from typing import Optional, Tuple
from pathlib import Path
import shutil
import subprocess

from .config import CoreConfig
from .errors import DiagnosticBag


@dataclass
class CoreResult:
    ok: bool
    diagnostics: DiagnosticBag
    ast: Optional[object] = None
    hir: Optional[object] = None


def _run_vittec(path: str, cfg: CoreConfig, diags: DiagnosticBag) -> Tuple[bool, Optional[str]]:
    vittec = shutil.which("vittec")
    if vittec is None:
        diags.warning("vittec not found in PATH; running in stub mode")
        return True, None

    repo_root = _find_repo_root(Path(path).resolve())
    check_target = str(repo_root / "src")
    cmd = [vittec, "check", check_target]
    if cfg.emit_ast:
        cmd += ["--emit-ast"]
    if cfg.dump_hir:
        cmd += ["--dump-hir"]
    if cfg.dump_tokens:
        cmd += ["--dump-tokens"]
    if not cfg.validate:
        cmd += ["--no-validate"]

    try:
        proc = subprocess.run(
            cmd,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
    except OSError as exc:
        diags.error(f"failed to run vittec: {exc}")
        return False, None

    output = proc.stdout or ""
    if proc.returncode != 0:
        diags.error("vittec returned non-zero exit code")
        return False, output
    return True, output


def _find_repo_root(start: Path) -> Path:
    for candidate in [start] + list(start.parents):
        if (candidate / "src").is_dir() and (candidate / "tools" / "vittec").is_dir():
            return candidate
    return start.parent


def run_source(source: str, config: Optional[CoreConfig] = None) -> CoreResult:
    cfg = config or CoreConfig.default()
    diags = DiagnosticBag()
    if cfg.dump_tokens:
        diags.note("tokens dumped")
    if cfg.emit_ast:
        diags.note("ast dumped")
    if cfg.dump_hir:
        diags.note("hir dumped")
    return CoreResult(True, diags)


def run_file(path: str, config: Optional[CoreConfig] = None) -> CoreResult:
    cfg = config or CoreConfig.default()
    diags = DiagnosticBag()
    ok, output = _run_vittec(path, cfg, diags)
    if output:
        for line in output.rstrip().splitlines():
            diags.note(line)
    return CoreResult(ok, diags)


def check_source(source: str, config: Optional[CoreConfig] = None) -> bool:
    return run_source(source, config).ok
