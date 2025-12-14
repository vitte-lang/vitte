from __future__ import annotations

from pathlib import Path
from typing import List, Set, Tuple

try:  # Python 3.11+
    import tomllib  # type: ignore[attr-defined]
except ModuleNotFoundError:  # pragma: no cover - fallback pour Python <3.11
    try:
        import tomli as tomllib  # type: ignore
    except ModuleNotFoundError:  # pragma: no cover - tomli absent
        tomllib = None  # type: ignore

DEFAULT_SOURCE_ROOTS = (
    "bootstrap",
    "compiler",
    "modules",
    "plugins",
    "src",
    "tests",
    "examples",
    "smoke",
)

DEFAULT_SOURCE_FILES = ("source.vitte",)


def _load_sources_table(project_manifest: Path) -> dict:
    if tomllib is None:
        return {}
    try:
        with project_manifest.open("rb") as handle:
            data = tomllib.load(handle)  # type: ignore[arg-type]
    except (FileNotFoundError, IsADirectoryError):
        return {}
    except Exception:
        return {}

    project_table = data.get("project")
    if not isinstance(project_table, dict):
        return {}

    sources_cfg = project_table.get("sources")
    if not isinstance(sources_cfg, dict):
        return {}

    return sources_cfg


def _as_str_list(value: object) -> List[str]:
    if not isinstance(value, list):
        return []
    result: List[str] = []
    for item in value:
        if isinstance(item, str) and item:
            result.append(item)
    return result


def resolve_source_entries(project_manifest: Path) -> Tuple[List[Path], List[Path]]:
    cfg = _load_sources_table(project_manifest)
    base_dir = project_manifest.parent

    roots = _as_str_list(cfg.get("roots"))
    files = _as_str_list(cfg.get("files"))

    if not roots:
        roots = list(DEFAULT_SOURCE_ROOTS)
    if not files:
        files = list(DEFAULT_SOURCE_FILES)

    root_paths = [(base_dir / Path(entry)).resolve() for entry in roots]
    file_paths = [(base_dir / Path(entry)).resolve() for entry in files]
    return root_paths, file_paths


def collect_project_sources(project_manifest: Path) -> List[Path]:
    """
    Retourne la liste des fichiers .vitte à compiler pour ce manifest Muffin.
    """
    root_paths, explicit_files = resolve_source_entries(project_manifest)

    sources: Set[Path] = set()

    for file_path in explicit_files:
        if file_path.suffix != ".vitte":
            continue
        if file_path.is_file():
            sources.add(file_path)

    for base in root_paths:
        if base.is_file() and base.suffix == ".vitte":
            sources.add(base)
            continue
        if not base.is_dir():
            continue
        for path in base.rglob("*.vitte"):
            sources.add(path)

    return sorted(sources)
