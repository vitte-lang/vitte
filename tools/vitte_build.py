#!/usr/bin/env python3
import argparse
import subprocess
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def run(cmd: list[str]):
    print("+", " ".join(cmd))
    subprocess.check_call(cmd)

def cmd_stage0(args):
    stage0_bin = ROOT / "bootstrap/bin/vittec-stage0"
    stage0_bin.parent.mkdir(parents=True, exist_ok=True)

    # Implémentation stage0 minimale :
    # on copie le compilateur précompilé target/bootstrap/stage1/vittec-stage1
    # vers bootstrap/bin/vittec-stage0. Cela évite d'inventer des flags
    # inexistants pour scripts/steelc et permet de stabiliser la chaîne.
    src = ROOT / "target/bootstrap/stage1/vittec-stage1"
    if not src.exists():
        raise SystemExit(
            f"[vitte_build][ERROR] Binaire bootstrap introuvable : {src}. "
            f"Vérifie que le repo contient bien ce fichier ou adapte cmd_stage0."
        )

    shutil.copy2(src, stage0_bin)

    if args.log:
        Path(args.log).parent.mkdir(parents=True, exist_ok=True)
        Path(args.log).write_text(
            f"stage0: copied {src} -> {stage0_bin}\n",
            encoding="utf-8",
        )

def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)

    p_stage0 = sub.add_parser("stage0")
    p_stage0.add_argument("--source", type=Path, required=True)
    p_stage0.add_argument("--out-bin", type=Path, required=True)
    p_stage0.add_argument("--log", type=Path, required=False)
    p_stage0.set_defaults(func=cmd_stage0)

    args = p.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
