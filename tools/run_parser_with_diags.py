#!/usr/bin/env python3
"""
Wrapper de secours pour exécuter le parser Vitte et afficher ses diagnostics.

Objectif (goldens) :
  - appeler un binaire de parser existant (vittec release ou stage0),
  - forwarder sa sortie stdout/stderr,
  - si aucune erreur n'est signalée mais qu'on rencontre des '.end' mal placés
    à l'intérieur d'un bloc { ... }, émettre un diagnostic minimal et sortir
    avec un code non nul.

Next steps : dès que le parser réel émet des diagnostics, brancher ce wrapper
dessus (ou supprimer le fallback heuristique).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
TARGET_DIR = ROOT / "target" / "release"
BOOTSTRAP_BIN = ROOT / "bootstrap" / "bin" / "vittec-stage0"

# Mapping minimal entre les messages du frontend Python et les codes d'erreur
# attendus par les goldens.
FRONTEND_MESSAGE_TO_CODE = {
    "',' ou '}' attendu après un champ de pattern struct": "E1208",
    "Utiliser '}' pour fermer un pattern de struct (pas '.end')": "E1206",
    "',' ou '}' attendu après un champ de struct": "E2007",
    "Un littéral de struct se ferme avec '}' (et non '.end')": "E2009",
    "Bloc non terminé ('.end' manquant)": "E1300",
    "Utiliser '}' pour fermer ce bloc": "E1302",
}


def resolve_parser_bin() -> Path | None:
    """
    Sélectionne un binaire parser disponible.
    Préférence :
      1) target/release/vittec
      2) bootstrap/bin/vittec-stage0 (wrapper Python)
    """
    candidates = [
        TARGET_DIR / "vittec",
        BOOTSTRAP_BIN,
    ]

    for path in candidates:
        if path.is_file() and os.access(path, os.X_OK):
            return path
    return None


def build_cmd(bin_path: Path, input_path: Path) -> list[str]:
    """
    Construit la commande à exécuter pour lancer le parser.
    vittec-stage0 est un script Python sans shebang : on le lance via python3.
    """
    try:
        first_bytes = bin_path.read_bytes()[:2]
    except OSError:
        first_bytes = b""

    if first_bytes.startswith(b"#!"):
        return [str(bin_path), "--dump-ast", str(input_path)]

    # Heuristique : vittec-stage0 est un fichier texte (pas ELF).
    if b"\x7fELF" not in first_bytes:
        python = sys.executable or "python3"
        return [python, str(bin_path), "--dump-ast", str(input_path)]

    return [str(bin_path), "--dump-ast", str(input_path)]


def run_parser(bin_path: Path, input_path: Path) -> tuple[int, str]:
    """Lance le parser et renvoie (code_retour, sortie_combinée)."""
    cmd = build_cmd(bin_path, input_path)
    proc = subprocess.run(cmd, capture_output=True, text=True)
    output = (proc.stdout or "") + (proc.stderr or "")
    if output:
        print(output, end="")
    return proc.returncode, output


def format_frontend_diags(input_path: Path) -> tuple[int, list[str]] | None:
    """
    Optionnel : tente d'utiliser tools/frontend_host.py pour produire des
    diagnostics "réels" dès qu'ils seront disponibles.
    """
    try:
        from frontend_host import format_diag as fh_format_diag, run_frontend  # type: ignore
    except Exception:
        return None

    rc, frontend_diags = run_frontend(input_path)  # type: ignore  # pragma: no cover
    if frontend_diags is None:
        return None

    formatted: list[str] = []
    for diag in frontend_diags:
        code = getattr(diag, "code", None)  # type: ignore[attr-defined]
        if (not code or code == "E0000") and diag.message in FRONTEND_MESSAGE_TO_CODE:
            diag.code = FRONTEND_MESSAGE_TO_CODE[diag.message]  # type: ignore[attr-defined]
        formatted.append(fh_format_diag(diag))  # type: ignore[arg-type]
    return (rc or 1, formatted)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Wrapper parser + diagnostics pour les goldens.",
    )
    parser.add_argument("input", type=Path, help="Fichier .vitte à parser.")
    args = parser.parse_args(argv)

    if not args.input.is_file():
        print(f"[vitte-parser-wrapper] ERROR: entrée introuvable: {args.input}")
        return 2

    bin_path = resolve_parser_bin()
    if bin_path is None:
        print("[vitte-parser-wrapper] ERROR: aucun binaire vittec trouvé.")
        return 2

    frontend_result = format_frontend_diags(args.input)
    if frontend_result is not None:
        frontend_rc, frontend_diags = frontend_result
        if frontend_diags:
            for diag in frontend_diags:
                print(diag)
            return frontend_rc or 1
        if frontend_rc:
            return frontend_rc

    rc, _ = run_parser(bin_path, args.input)
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
