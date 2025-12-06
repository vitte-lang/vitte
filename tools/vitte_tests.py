#!/usr/bin/env python3
"""
vitte_tests.py — Runner de tests pour le projet vitte-core.

Objectifs :
  - Fournir une CLI de tests *dev-only* en Python, qui pilote les outils Vitte
    existants sans jamais les remplacer.
  - Regrouper plusieurs familles de tests :
      * smoke     : lance les tests de fumée dans tests/smoke/ via steelc,
      * malformed : vérifie que les fichiers invalides provoquent bien des erreurs,
      * goldens   : délègue à scripts/run_goldens.sh,
      * all       : exécute l'ensemble des suites ci-dessus.

Contraintes :
  - Python n'exécute aucun morceau de compilateur : il ne fait qu'orchestrer
    les binaires / scripts existants (vittec, steelc, run_goldens.sh).
  - Ce script n'est pas requis pour les utilisateurs finaux de Vitte : il est
    destiné aux développeurs du projet vitte-core.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPTS_DIR = ROOT / "scripts"
TOOLS_DIR = ROOT / "tools"
TARGET_DIR = ROOT / "target"

STEELC = SCRIPTS_DIR / "steelc"
VITTEC_RELEASE = TARGET_DIR / "release" / "vittec"
RUN_GOLDENS_SH = SCRIPTS_DIR / "run_goldens.sh"

TESTS_DIR = ROOT / "tests"
SMOKE_DIR = TESTS_DIR / "smoke"
MALFORMED_DIR = TESTS_DIR / "malformed"


# ---------------------------------------------------------------------------
# Helpers génériques
# ---------------------------------------------------------------------------

def run_cmd(cmd: list[str], cwd: Path | None = None) -> int:
    """Exécute une commande et renvoie le code de retour."""
    cwd = cwd or ROOT
    print(f"[vitte-tests] run: {' '.join(cmd)} (cwd={cwd})")
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            check=False,
        )
        return result.returncode
    except FileNotFoundError:
        print(f"[vitte-tests] ERROR: commande introuvable: {cmd[0]}")
        return 127


def ensure_executable(path: Path, kind: str) -> bool:
    """
    Vérifie qu'un binaire ou script est présent et exécutable.

    kind: description humaine ("steelc", "vittec", "run_goldens.sh", etc.)
    """
    if path.is_file() and os.access(path, os.X_OK):
        return True

    print(f"[vitte-tests] ERROR: {kind} introuvable ou non exécutable: {path}")
    return False


def ensure_steelc() -> bool:
    """Vérifie la présence du script steelc."""
    return ensure_executable(STEELC, "script steelc")


def ensure_vittec_release() -> bool:
    """Vérifie la présence du compilateur release vittec (target/release/vittec)."""
    return ensure_executable(VITTEC_RELEASE, "compilateur release vittec")


# ---------------------------------------------------------------------------
# Commandes de tests
# ---------------------------------------------------------------------------

def cmd_smoke(args: argparse.Namespace) -> int:
    """
    Exécute les tests 'smoke' du dossier tests/smoke/ via scripts/steelc.

    Chaque fichier *.vitte de tests/smoke est lancé séquentiellement.
    Un test est considéré comme réussi si le code de retour est 0.
    """
    if not ensure_steelc():
        return 1

    if not SMOKE_DIR.is_dir():
        print(f"[vitte-tests] WARNING: pas de dossier de tests smoke: {SMOKE_DIR}")
        return 0

    tests = sorted(SMOKE_DIR.glob("*.vitte"))
    if not tests:
        print(f"[vitte-tests] WARNING: aucun test *.vitte dans {SMOKE_DIR}")
        return 0

    print(f"[vitte-tests] running {len(tests)} smoke test(s)…")
    ok = 0
    for t in tests:
        rel = t.relative_to(ROOT)
        cmd = [str(STEELC), str(rel)]
        rc = run_cmd(cmd, cwd=ROOT)
        if rc == 0:
            ok += 1
        else:
            print(f"[vitte-tests] FAILED (smoke): {rel} (exit {rc})")
    print(f"[vitte-tests] smoke: {ok}/{len(tests)} passed")
    return 0 if ok == len(tests) else 1


def cmd_malformed(args: argparse.Namespace) -> int:
    """
    Exécute les tests 'malformed' du dossier tests/malformed/ via vittec release.

    Convention :
      - Chaque fichier dans tests/malformed/ doit provoquer un échec du compilateur.
      - Un test est considéré comme réussi si le code de retour est NON nul.
    """
    if not ensure_vittec_release():
        return 1

    if not MALFORMED_DIR.is_dir():
        print(f"[vitte-tests] WARNING: pas de dossier de tests malformed: {MALFORMED_DIR}")
        return 0

    tests = sorted(MALFORMED_DIR.iterdir())
    tests = [t for t in tests if t.is_file()]

    if not tests:
        print(f"[vitte-tests] WARNING: aucun fichier dans {MALFORMED_DIR}")
        return 0

    print(f"[vitte-tests] running {len(tests)} malformed test(s)…")
    ok = 0
    for t in tests:
        rel = t.relative_to(ROOT)
        cmd = [str(VITTEC_RELEASE), str(rel)]
        rc = run_cmd(cmd, cwd=ROOT)
        if rc != 0:
            ok += 1
        else:
            print(f"[vitte-tests] FAILED (malformed): {rel} (exit {rc}, attendu != 0)")
    print(f"[vitte-tests] malformed: {ok}/{len(tests)} passed")
    return 0 if ok == len(tests) else 1


def cmd_goldens(args: argparse.Namespace) -> int:
    """
    Délègue l'exécution des tests goldens à scripts/run_goldens.sh.

    La logique de comparaison des goldens reste dans le script shell,
    Python se contente de l'orchestration.
    """
    if not ensure_executable(RUN_GOLDENS_SH, "scripts/run_goldens.sh"):
        return 1

    cmd = [str(RUN_GOLDENS_SH)]
    rc = run_cmd(cmd, cwd=ROOT)
    if rc == 0:
        print("[vitte-tests] goldens: OK")
    else:
        print(f"[vitte-tests] goldens: FAILED (exit {rc})")
    return rc


def cmd_all(args: argparse.Namespace) -> int:
    """
    Exécute toutes les familles de tests dans l'ordre :
      1) smoke
      2) malformed
      3) goldens
    """
    rc_smoke = cmd_smoke(args)
    rc_malformed = cmd_malformed(args)
    rc_goldens = cmd_goldens(args)

    if rc_smoke == 0 and rc_malformed == 0 and rc_goldens == 0:
        print("[vitte-tests] ALL TESTS PASSED")
        return 0

    print("[vitte-tests] SOME TESTS FAILED:")
    print(f"  smoke     -> {'OK' if rc_smoke == 0 else 'FAIL'}")
    print(f"  malformed -> {'OK' if rc_malformed == 0 else 'FAIL'}")
    print(f"  goldens   -> {'OK' if rc_goldens == 0 else 'FAIL'}")
    return 1


# ---------------------------------------------------------------------------
# CLI argparse
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="vitte_tests",
        description="Runner de tests (dev-only) pour le projet vitte-core.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # vitte_tests.py smoke
    p_smoke = sub.add_parser(
        "smoke",
        help="Exécute les tests de fumée dans tests/smoke/ via steelc.",
    )
    p_smoke.set_defaults(func=cmd_smoke)

    # vitte_tests.py malformed
    p_malformed = sub.add_parser(
        "malformed",
        help="Exécute les tests malformed dans tests/malformed/ (doivent échouer).",
    )
    p_malformed.set_defaults(func=cmd_malformed)

    # vitte_tests.py goldens
    p_goldens = sub.add_parser(
        "goldens",
        help="Exécute les tests goldens via scripts/run_goldens.sh.",
    )
    p_goldens.set_defaults(func=cmd_goldens)

    # vitte_tests.py all
    p_all = sub.add_parser(
        "all",
        help="Exécute l'ensemble des tests : smoke, malformed, goldens.",
    )
    p_all.set_defaults(func=cmd_all)

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
