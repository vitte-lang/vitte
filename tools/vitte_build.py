#!/usr/bin/env python3
"""
Outil de build et d'exécution pour le projet vitte-core.

Objectifs :
  - Fournir une petite CLI Python pour piloter le compilateur Vitte existant,
    sans jamais le remplacer (Python reste un outil d'atelier).
  - Simplifier les actions fréquentes :
      * construire le compilateur (make release),
      * lancer vittec sur un fichier .vitte,
      * exécuter la suite de tests smoke.

Ce script n'est pas requis pour les utilisateurs finaux de Vitte.
Il sert uniquement pour le développement du projet vitte-core.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPTS_DIR = ROOT / "scripts"
TARGET_DIR = ROOT / "target"
VITTEC_RELEASE = TARGET_DIR / "release" / "vittec"
VITTEC_STAGE1 = TARGET_DIR / "bootstrap" / "stage1" / "vittec-stage1"
TESTS_SMOKE_DIR = ROOT / "tests" / "smoke"


def run_cmd(cmd: list[str], cwd: Path | None = None) -> int:
    """Exécute une commande et renvoie le code de retour."""
    cwd = cwd or ROOT
    print(f"[vitte-build] run: {' '.join(cmd)} (cwd={cwd})")
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            check=False,
        )
        return result.returncode
    except FileNotFoundError:
        print(f"[vitte-build] ERROR: commande introuvable: {cmd[0]}")
        return 127


def ensure_vittec_release() -> bool:
    """
    Vérifie que target/release/vittec existe.

    Si le binaire n'existe pas, suggère à l'utilisateur d'exécuter `make release`.
    """
    if VITTEC_RELEASE.is_file() and os.access(VITTEC_RELEASE, os.X_OK):
        return True

    print(f"[vitte-build] ERROR: compilateur release introuvable: {VITTEC_RELEASE}")
    print("[vitte-build]       Construis-le d'abord, par exemple :")
    print("           cd", ROOT)
    print("           make release")
    return False


def cmd_build(args: argparse.Namespace) -> int:
    """
    Construit le compilateur self-host via `make release`.
    """
    cmd = ["make", "release"]
    return run_cmd(cmd, cwd=ROOT)


def cmd_run(args: argparse.Namespace) -> int:
    """
    Lance vittec (release) sur un fichier .vitte avec des arguments bruts.

    Exemples :
      vitte_build.py run source.vitte
      vitte_build.py run -- --dump-json source.vitte
    """
    if not ensure_vittec_release():
        return 1

    vittec = str(VITTEC_RELEASE)
    extra = args.vittec_args or []
    cmd = [vittec, *extra]
    return run_cmd(cmd, cwd=ROOT)


def cmd_smoke(args: argparse.Namespace) -> int:
    """
    Exécute les tests 'smoke' du dossier tests/smoke/ avec le compilateur release.

    Chaque fichier *.vitte de tests/smoke est lancé séquentiellement.
    """
    if not ensure_vittec_release():
        return 1

    if not TESTS_SMOKE_DIR.is_dir():
        print(f"[vitte-build] WARNING: pas de dossier de tests smoke: {TESTS_SMOKE_DIR}")
        return 0

    vittec = str(VITTEC_RELEASE)
    tests = sorted(TESTS_SMOKE_DIR.glob("*.vitte"))
    if not tests:
        print(f"[vitte-build] WARNING: aucun test *.vitte dans {TESTS_SMOKE_DIR}")
        return 0

    print(f"[vitte-build] running {len(tests)} smoke test(s)…")
    ok = 0
    for t in tests:
        rel = t.relative_to(ROOT)
        cmd = [vittec, str(rel)]
        rc = run_cmd(cmd, cwd=ROOT)
        if rc == 0:
            ok += 1
        else:
            print(f"[vitte-build] FAILED: {rel} (exit {rc})")
    print(f"[vitte-build] {ok}/{len(tests)} smoke test(s) passed")
    return 0 if ok == len(tests) else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="vitte_build",
        description="Outil de build/exec pour le projet vitte-core (dev-only, Python + Vitte).",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # vitte_build.py build
    p_build = sub.add_parser(
        "build",
        help="Construit le compilateur Vitte self-host (alias `make release`).",
    )
    p_build.set_defaults(func=cmd_build)

    # vitte_build.py run [-- ...args vers vittec...]
    p_run = sub.add_parser(
        "run",
        help="Lance target/release/vittec avec des arguments passés tels quels.",
    )
    p_run.add_argument(
        "vittec_args",
        nargs=argparse.REMAINDER,
        help="Arguments à passer directement à vittec (ex: source.vitte, --dump-json, etc.).",
    )
    p_run.set_defaults(func=cmd_run)

    # vitte_build.py smoke
    p_smoke = sub.add_parser(
        "smoke",
        help="Exécute les tests smoke du dossier tests/smoke/ avec vittec (release).",
    )
    p_smoke.set_defaults(func=cmd_smoke)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    func = getattr(args, "func", None)
    if func is None:
        parser.print_help()
        return 1
    return func(args)


if __name__ == "__main__":
    raise SystemExit(main())
