

# =========================================================
# Vitte — Bootstrap / Configure
# =========================================================
"""
Configuration et initialisation de l’environnement Python
pour le bootstrap du langage Vitte.

Rôle :
- préparer sys.path pour importer le noyau Vitte
- vérifier l’environnement d’exécution
- centraliser la configuration bootstrap
- servir de point commun pour bootstrap, tests et LSP

Ce module ne compile rien.
"""

from __future__ import annotations

import sys
from pathlib import Path
from dataclasses import dataclass
from typing import Optional


# ---------------------------------------------------------
# Informations de configuration
# ---------------------------------------------------------

@dataclass
class BootstrapConfig:
    project_root: Path
    src_dir: Path
    core_dir: Path
    verbose: bool = False


# ---------------------------------------------------------
# Détection du projet
# ---------------------------------------------------------

def detect_project_root(start: Optional[Path] = None) -> Path:
    """
    Détecte la racine du projet Vitte en remontant l’arborescence.
    """
    current = start or Path(__file__).resolve()

    for parent in [current, *current.parents]:
        if (parent / "src").exists() and (parent / "README.md").exists():
            return parent

    raise RuntimeError("unable to locate Vitte project root")


# ---------------------------------------------------------
# Construction configuration bootstrap
# ---------------------------------------------------------

def make_config(verbose: bool = False) -> BootstrapConfig:
    root = detect_project_root()
    src = root / "src"
    core = src / "core"

    if not core.exists():
        raise RuntimeError("core directory not found (src/core)")

    return BootstrapConfig(
        project_root=root,
        src_dir=src,
        core_dir=core,
        verbose=verbose,
    )


# ---------------------------------------------------------
# Initialisation environnement Python
# ---------------------------------------------------------

def setup_python_path(cfg: BootstrapConfig) -> None:
    """
    Ajoute src/ au PYTHONPATH pour permettre les imports Vitte.
    """
    src_str = str(cfg.src_dir)

    if src_str not in sys.path:
        sys.path.insert(0, src_str)
        if cfg.verbose:
            print(f"[bootstrap] added to sys.path: {src_str}")


# ---------------------------------------------------------
# Vérifications environnement
# ---------------------------------------------------------

def check_environment(cfg: BootstrapConfig) -> None:
    """
    Vérifie la cohérence minimale de l’environnement bootstrap.
    """
    if not cfg.project_root.exists():
        raise RuntimeError("project root does not exist")

    if not cfg.src_dir.exists():
        raise RuntimeError("src directory missing")

    if not cfg.core_dir.exists():
        raise RuntimeError("core directory missing")


# ---------------------------------------------------------
# Initialisation complète
# ---------------------------------------------------------

def configure(verbose: bool = False) -> BootstrapConfig:
    """
    Initialise complètement l’environnement bootstrap Vitte.
    """
    cfg = make_config(verbose=verbose)
    check_environment(cfg)
    setup_python_path(cfg)

    if cfg.verbose:
        print("[bootstrap] configuration complete")
        print(f"  root : {cfg.project_root}")
        print(f"  src  : {cfg.src_dir}")
        print(f"  core : {cfg.core_dir}")

    return cfg