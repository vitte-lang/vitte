#!/usr/bin/env python3
"""
Vitte bootstrap – vittec_stage1

CLI host de référence pour le compilateur Vitte. Cette version est volontairement
autonome et structurée, pour servir de contrat :

- Subcommands haut niveau :
    vittec build --project <muffin.muf> [--out-bin <bin>] [--log-file <log>]
    vittec check --project <muffin.muf> [--log-file <log>]

- Modes "outil" sur fichier unique :
    vittec --dump-tokens <file.vitte>
    vittec --dump-ast <file.vitte>
    vittec --dump-json <file.vitte>

- Mode historique (single file) :
    vittec <file.vitte>

Cette implémentation ne fait PAS le vrai travail de compilation : tout est en
mode "TODO" structuré, prêt à être remplacé par la version Vitte.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Sequence


PROG_NAME = "vittec"
VERSION = "0.1.0-bootstrap"


# -----------------------------------------------------------------------------
# Modèles de données pour les commandes
# -----------------------------------------------------------------------------

@dataclass
class ProjectCommandOptions:
    project: Path
    out_bin: Optional[Path]
    log_file: Optional[Path]


@dataclass
class DumpCommandOptions:
    input_file: Path


# -----------------------------------------------------------------------------
# Construction du parser
# -----------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=PROG_NAME,
        description="Vitte compiler host (bootstrap stage1 CLI).",
    )

    parser.add_argument(
        "--version",
        action="store_true",
        help="Affiche la version de vittec.",
    )

    # Modes "outil" (compat historique + future CLI Vitte)
    parser.add_argument(
        "--dump-json",
        action="store_true",
        help="Dump JSON représentant le fichier source (mode historique / tool).",
    )
    parser.add_argument(
        "--dump-tokens",
        action="store_true",
        help="Dump la liste des tokens lexés pour un fichier .vitte.",
    )
    parser.add_argument(
        "--dump-ast",
        action="store_true",
        help="Dump l'AST du fichier source .vitte.",
    )

    # Subcommands build / check
    subparsers = parser.add_subparsers(
        dest="subcommand",
        title="subcommands",
        metavar="{build,check}",
    )

    # vittec build --project ... [--out-bin ...] [--log-file ...]
    p_build = subparsers.add_parser(
        "build",
        help="Construit un projet Vitte à partir d'un manifest Muffin.",
    )
    p_build.add_argument(
        "--project",
        type=Path,
        required=True,
        help="Chemin du manifest Muffin (.muf) du projet à construire.",
    )
    p_build.add_argument(
        "--out-bin",
        type=Path,
        required=False,
        help="Chemin du binaire de sortie (optionnel).",
    )
    p_build.add_argument(
        "--log-file",
        type=Path,
        required=False,
        help="Chemin d'un fichier log à écrire (optionnel).",
    )

    # vittec check --project ... [--log-file ...]
    p_check = subparsers.add_parser(
        "check",
        help="Vérifie un projet Vitte (analyse, typechecking, sans build final).",
    )
    p_check.add_argument(
        "--project",
        type=Path,
        required=True,
        help="Chemin du manifest Muffin (.muf) du projet à vérifier.",
    )
    p_check.add_argument(
        "--log-file",
        type=Path,
        required=False,
        help="Chemin d'un fichier log à écrire (optionnel).",
    )

    # Argument positionnel pour les modes "outil" / historique
    parser.add_argument(
        "input",
        nargs="?",
        type=Path,
        help="Fichier source .vitte pour les modes dump-* ou compilation simple.",
    )

    return parser


# -----------------------------------------------------------------------------
# Fonctions utilitaires
# -----------------------------------------------------------------------------

def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        print(f"{PROG_NAME}: erreur de lecture fichier {path}: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


def write_text(path: Path, content: str) -> None:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
    except OSError as exc:
        print(f"{PROG_NAME}: erreur d'écriture fichier {path}: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


def log_if_needed(log_file: Optional[Path], message: str) -> None:
    if not log_file:
        return
    try:
        with log_file.open("a", encoding="utf-8") as f:
            f.write(message.rstrip("\n") + "\n")
    except OSError as exc:
        print(f"{PROG_NAME}: impossible d'é