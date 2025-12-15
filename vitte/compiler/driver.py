from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

from tools.frontend_host import Diagnostic, Span, run_frontend
from tools.project_sources import collect_project_sources


@dataclass
class DriverResult:
    """Résultat standard du driver Vitte."""

    exit_code: int
    diagnostics: List[Diagnostic]


def _log(log_file: Optional[Path], message: str) -> None:
    if not log_file:
        return
    try:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        with log_file.open("a", encoding="utf-8") as f:
            f.write(message.rstrip("\n") + "\n")
    except OSError:
        # On ignore les erreurs de log pour ne pas masquer l'erreur principale.
        return


def _collect_sources(project_manifest: Path) -> List[Path]:
    return collect_project_sources(project_manifest)


def _manifest_missing_diag(project_manifest: Path) -> Diagnostic:
    return Diagnostic(
        message="Manifest Muffin introuvable",
        span=Span(project_manifest, 1, 1),
        code="E0001",
    )


def _wrap_internal_error(exc: Exception, project: Path) -> Diagnostic:
    return Diagnostic(
        message=f"Erreur interne du driver: {exc}",
        span=Span(project, 0, 0),
        code="E9999",
    )


def _run_frontend_on_sources(sources: Iterable[Path]) -> Tuple[int, List[Diagnostic]]:
    all_diags: List[Diagnostic] = []
    for src in sources:
        _rc, diags = run_frontend(src)
        all_diags.extend(diags)
    return (1 if all_diags else 0), all_diags


def run_build(project_path: Path, out_bin: Optional[Path], log_file: Optional[Path]) -> DriverResult:
    """
    API Python du driver Vitte — build complet (frontend only pour l'instant).
    """
    try:
        project_manifest = Path(project_path)
        if not project_manifest.is_file():
            diag = _manifest_missing_diag(project_manifest)
            return DriverResult(exit_code=1, diagnostics=[diag])

        sources = _collect_sources(project_manifest)
        _rc, diags = _run_frontend_on_sources(sources)
        exit_code = 1 if diags else 0
        _log(
            log_file,
            f"[vitte.driver] build project={project_manifest} sources={len(sources)} exit={exit_code}",
        )
        return DriverResult(exit_code=exit_code, diagnostics=diags)
    except Exception as exc:  # pragma: no cover - garde-fou
        diag = _wrap_internal_error(exc, Path(project_path))
        return DriverResult(exit_code=2, diagnostics=[diag])


def run_check(project_path: Path, log_file: Optional[Path]) -> DriverResult:
    """
    API Python du driver Vitte — check (frontend uniquement pour l'instant).
    """
    try:
        project_manifest = Path(project_path)
        if not project_manifest.is_file():
            diag = _manifest_missing_diag(project_manifest)
            return DriverResult(exit_code=1, diagnostics=[diag])

        sources = _collect_sources(project_manifest)
        _rc, diags = _run_frontend_on_sources(sources)
        exit_code = 1 if diags else 0
        _log(
            log_file,
            f"[vitte.driver] check project={project_manifest} sources={len(sources)} exit={exit_code}",
        )
        return DriverResult(exit_code=exit_code, diagnostics=diags)
    except Exception as exc:  # pragma: no cover - garde-fou
        diag = _wrap_internal_error(exc, Path(project_path))
        return DriverResult(exit_code=2, diagnostics=[diag])
