#!/usr/bin/env python3
from pathlib import Path
from typing import Optional, Sequence
import argparse
import sys
from dataclasses import dataclass
import json

@dataclass
class ProjectCommandOptions:
    """
    Options normalisées pour les commandes orientées projet (build / check).
    """
    project: Path
    out_bin: Optional[Path]
    log_file: Optional[Path]


def log_if_needed(log_file: Optional[Path], message: str) -> None:
    """
    Écrit `message` dans le fichier de log si `log_file` est défini.
    Sinon, ne fait rien.
    """
    if not log_file:
        return
    try:
        with log_file.open("a", encoding="utf-8") as f:
            f.write(message.rstrip("\n") + "\n")
    except OSError as exc:
        print(
            f"vittec: impossible d'écrire dans log {log_file}: {exc}",
            file=sys.stderr,
        )


def write_text(path: Path, content: str) -> None:
    """
    Helper simple pour écrire du texte dans un fichier en UTF-8.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def find_vitte_sources(project_manifest: Path) -> list[Path]:
    """
    Retourne la liste des fichiers .vitte pertinents pour le projet,
    en scannant les répertoires bootstrap/, compiler/ et src/ à côté
    du manifest Muffin.
    """
    root = project_manifest.parent
    sources: list[Path] = []

    for subdir_name in ("bootstrap", "compiler", "src"):
        base = root / subdir_name
        if not base.is_dir():
            continue
        for path in base.rglob("*.vitte"):
            sources.append(path)

    # Tri pour des résultats déterministes
    sources.sort()
    return sources


def create_placeholder_binary(path: Path, project: Path) -> None:
    script = """#!/usr/bin/env sh
echo "[vittec][bootstrap] binaire fictif pour {project}"
echo "Ce fichier a été généré par le bootstrap Python (stage1)."
echo "Aucune compilation réelle n'a été effectuée."
exit 0
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(script.format(project=project), encoding="utf-8")
    path.chmod(0o755)


def run_vitte_driver(
    mode: str,
    project: Path,
    out_bin: Optional[Path],
    log_file: Optional[Path],
) -> int:
    """
    Point d'unification pour appeler le driver Vitte.

    mode     : "build" ou "check"
    project  : chemin vers le manifest .muf
    out_bin  : binaire de sortie (build), None pour check
    log_file : log à produire (optionnel)

    Contrat codes retour :
      0 -> succès
      1 -> erreurs de compilation utilisateur
      2 -> erreur interne du compilateur
    """
    project = project.resolve()
    out_bin_resolved = out_bin.resolve() if out_bin is not None else None
    log_path = log_file.resolve() if log_file is not None else None

    # TODO (plus tard) : appel réel à la VM / runtime Vitte pour exécuter
    # vitte.compiler.driver.run_build / run_check.

    if mode not in ("build", "check"):
        msg = f"[vittec][host] run_vitte_driver: mode invalide: {mode!r}"
        print(msg)
        log_if_needed(log_file, msg)
        return 2

    msg = (
        f"[vittec][host] run_vitte_driver(mode={mode}, "
        f"project={project}, out_bin={out_bin_resolved}, log={log_path})\n"
        "  (TODO: remplacer par un appel au module vitte.compiler.driver)"
    )
    print(msg)
    log_if_needed(log_file, msg)

    # Tant que le driver Vitte n'est pas câblé, on considère que ça réussit.
    return 0


def cmd_build_project(opts: ProjectCommandOptions) -> int:
    root = opts.project.parent

    # 1) Scanner les sources Vitte
    sources = find_vitte_sources(opts.project)
    rel_sources = [str(p.relative_to(root)) for p in sources]

    msg = (
        f"[vittec][bootstrap] build projet (Muffin) : {opts.project}\n"
        f"  root     = {root}\n"
        f"  out-bin  = {opts.out_bin}\n"
        f"  log-file = {opts.log_file}\n"
        f"  sources  = {len(sources)} fichiers .vitte trouvés sous bootstrap/, compiler/, src/"
    )
    print(msg)
    log_if_needed(opts.log_file, msg)

    # 2) Appel contractuel au driver Vitte (futur point unique)
    code = run_vitte_driver(
        mode="build",
        project=opts.project,
        out_bin=opts.out_bin,
        log_file=opts.log_file,
    )

    # 3) Si le driver signale une erreur, on propage immédiatement
    if code != 0:
        # Pas de placeholder en cas d'échec du compilateur.
        return code

    # 4) Bootstrap actuel : .sources.json + placeholder binaire en cas de succès
    if opts.out_bin is not None:
        meta = {
            "project": str(opts.project),
            "root": str(root),
            "sources": rel_sources,
        }
        meta_path = opts.out_bin.with_suffix(".sources.json")
        write_text(meta_path, json.dumps(meta, indent=2))
        log_if_needed(
            opts.log_file,
            f"[vittec][bootstrap] sources list écrite dans {meta_path}",
        )
        print(f"[vittec][bootstrap] sources list écrite dans {meta_path}")

        create_placeholder_binary(opts.out_bin, opts.project)
        log_if_needed(
            opts.log_file,
            f"[vittec][bootstrap] placeholder écrit dans {opts.out_bin}",
        )
        print(f"[vittec][bootstrap] placeholder écrit dans {opts.out_bin}")

    # 5) Retourner le code du driver (0 ici tant que le driver est stub)
    return code


def cmd_check_project(opts: ProjectCommandOptions) -> int:
    root = opts.project.parent
    sources = find_vitte_sources(opts.project)
    msg = (
        f"[vittec][bootstrap] check projet (Muffin) : {opts.project}\n"
        f"  root     = {root}\n"
        f"  log-file = {opts.log_file}\n"
        f"  sources  = {len(sources)} fichiers .vitte trouvés sous bootstrap/, compiler/, src/\n"
        "  (implémentation réelle à écrire en Vitte : parse + resolve + typecheck.)"
    )
    print(msg)
    log_if_needed(opts.log_file, msg)

    code = run_vitte_driver(
        mode="check",
        project=opts.project,
        out_bin=None,
        log_file=opts.log_file,
    )

    return code


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="vittec-stage1",
        description="Stub bootstrap vittec (stage1).",
    )
    parser.add_argument(
        "--version",
        action="store_true",
        help="Affiche la version du stub stage1.",
    )
    sub = parser.add_subparsers(dest="command")

    p_build = sub.add_parser("build", help="Construit un projet (stub).")
    p_build.add_argument("project", type=Path)
    p_build.add_argument("--out-bin", type=Path)
    p_build.add_argument("--log-file", type=Path)

    p_check = sub.add_parser("check", help="Vérifie un projet (stub).")
    p_check.add_argument("project", type=Path)
    p_check.add_argument("--log-file", type=Path)

    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.version:
        print("vittec-stage1 0.1.0-bootstrap (stub)")
        return 0

    if args.command == "build":
        opts = ProjectCommandOptions(
            project=args.project,
            out_bin=args.out_bin,
            log_file=args.log_file,
        )
        return cmd_build_project(opts)

    if args.command == "check":
        opts = ProjectCommandOptions(
            project=args.project,
            out_bin=None,
            log_file=args.log_file,
        )
        return cmd_check_project(opts)

    parser.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
