"""vitte_benchgen

Vitte benchmark/corpus generator package.

This package is used by `compiler/bench/src/data/generators/*` to build deterministic
fixtures and corpora for:
- micro/bench suites
- parser/lexer/sema stress corpora
- compatibility test data

Design goals:
- Deterministic outputs (stable ordering + stable hashing)
- CI-friendly (clear diagnostics, stable exit codes)
- Minimal dependencies (stdlib-first)

Public API is intentionally small and uses lazy imports so that tooling can
reference `vitte_benchgen` even when optional submodules are not present.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Dict, Iterable, List, Optional, Sequence, Tuple, TYPE_CHECKING


# -----------------------------------------------------------------------------
# Version
# -----------------------------------------------------------------------------

def _detect_version() -> str:
    """Best-effort package version.

    - If installed as a distribution, uses importlib.metadata.
    - Otherwise returns a local placeholder.
    """

    try:
        from importlib.metadata import PackageNotFoundError, version  # type: ignore

        try:
            return version("vitte_benchgen")
        except PackageNotFoundError:
            # Sometimes the dist name differs from import name.
            for dist in ("vitte-benchgen", "vitte_benchgen"):
                try:
                    return version(dist)
                except PackageNotFoundError:
                    pass
    except Exception:
        pass

    return "0.0.0+local"


__version__ = _detect_version()


def get_version() -> str:
    """Return the package version string."""

    return __version__


# -----------------------------------------------------------------------------
# Lightweight shared types
# -----------------------------------------------------------------------------


@dataclass(frozen=True)
class ManifestEntry:
    """Generic manifest entry used by generators.

    Not all generators produce all fields.
    """

    path: str
    bytes: int = 0
    sha256: str = ""


# -----------------------------------------------------------------------------
# Lazy exports
# -----------------------------------------------------------------------------


def _safe_import(module: str) -> Optional[Any]:
    try:
        return __import__(module, fromlist=["*"])
    except Exception:
        return None


# PEP 562: module-level lazy attribute access.
# We keep this mapping small and stable.
_LAZY: Dict[str, Tuple[str, str]] = {
    # name -> (module, attr)
    "BenchGen": ("vitte_benchgen.benchgen", "BenchGen"),
    "GeneratorConfig": ("vitte_benchgen.config", "GeneratorConfig"),
    "load_config": ("vitte_benchgen.config", "load_config"),
    "generate": ("vitte_benchgen.benchgen", "generate"),
    "write_manifest": ("vitte_benchgen.manifest", "write_manifest"),
    "read_manifest": ("vitte_benchgen.manifest", "read_manifest"),
    "sha256_file": ("vitte_benchgen.manifest", "sha256_file"),
    "main": ("vitte_benchgen.cli", "main"),
}


def __getattr__(name: str) -> Any:
    spec = _LAZY.get(name)
    if spec is None:
        raise AttributeError(name)

    mod_name, attr = spec
    mod = _safe_import(mod_name)
    if mod is None:
        raise AttributeError(f"{name} (missing optional module '{mod_name}')")

    try:
        v = getattr(mod, attr)
    except AttributeError as e:
        raise AttributeError(f"{name} (missing attribute '{attr}' in '{mod_name}')") from e

    globals()[name] = v
    return v


def __dir__() -> List[str]:
    return sorted(list(globals().keys()) + list(_LAZY.keys()))


__all__ = [
    "__version__",
    "get_version",
    "ManifestEntry",
    # lazy exports
    "BenchGen",
    "GeneratorConfig",
    "load_config",
    "generate",
    "read_manifest",
    "write_manifest",
    "sha256_file",
    "main",
]


# -----------------------------------------------------------------------------
# Type-checker friendly imports
# -----------------------------------------------------------------------------

if TYPE_CHECKING:
    # These imports may not exist in minimal setups; only for IDE/type checking.
    from vitte_benchgen.benchgen import BenchGen as BenchGen  # noqa: F401
    from vitte_benchgen.benchgen import generate as generate  # noqa: F401
    from vitte_benchgen.cli import main as main  # noqa: F401
    from vitte_benchgen.config import GeneratorConfig as GeneratorConfig  # noqa: F401
    from vitte_benchgen.config import load_config as load_config  # noqa: F401
    from vitte_benchgen.manifest import read_manifest as read_manifest  # noqa: F401
    from vitte_benchgen.manifest import sha256_file as sha256_file  # noqa: F401
    from vitte_benchgen.manifest import write_manifest as write_manifest  # noqa: F401
