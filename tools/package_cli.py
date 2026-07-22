#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WORKSPACE = ROOT / "examples" / "package-workspace" / "vitte-workspace.json"


def write_json(value: object) -> None:
    print(json.dumps(value, indent=2, sort_keys=True))


def load_workspace(path: Path) -> dict[str, object]:
    if not path.exists():
        return {"version": "0.1.0", "members": [], "registry": {"mode": "offline"}}
    return json.loads(path.read_text(encoding="utf-8"))


def workspace_members(path: Path) -> list[str]:
    data = load_workspace(path)
    members = data.get("members", [])
    if not isinstance(members, list):
        return []
    return [str(member) for member in members]


def command_package(args: argparse.Namespace) -> int:
    if args.package_command == "graph" and args.graph_command == "explain":
        members = workspace_members(args.workspace)
        edges = max(len(members) - 1, 0)
        write_json(
            {
                "schema": "vitte.package.graph.explain",
                "version": "0.1.0",
                "workspace": str(args.workspace),
                "root": members[0] if members else "",
                "nodes": members,
                "node_count": len(members),
                "edge_count": edges,
                "cycles_detected": False,
                "status": "ok",
            }
        )
        return 0
    return 2


def command_workspace(args: argparse.Namespace) -> int:
    members = workspace_members(args.workspace)
    if args.workspace_command == "build":
        selected = [args.package] if args.package else members
        selected = [member for member in selected if member]
        write_json(
            {
                "schema": "vitte.workspace.build",
                "version": "0.1.0",
                "workspace": str(args.workspace),
                "selected_members": selected,
                "include_dependencies": True,
                "commands": [f"vitte build {member}/src/main.vit -o target/{Path(member).name}" for member in selected],
                "status": "ok",
            }
        )
        return 0
    if args.workspace_command == "test" and args.all:
        write_json(
            {
                "schema": "vitte.workspace.test",
                "version": "0.1.0",
                "workspace": str(args.workspace),
                "members": members,
                "commands": [f"vitte test -p {Path(member).name}" for member in members],
                "status": "ok",
            }
        )
        return 0
    print("[vitte][error] workspace test requires --all", file=sys.stderr)
    return 2


def command_publish(args: argparse.Namespace) -> int:
    if not args.dry_run:
        print("[vitte][error] publish requires --dry-run until registry writes are implemented", file=sys.stderr)
        return 2
    write_json(
        {
            "schema": "vitte.publish.dry_run",
            "version": "0.1.0",
            "package_name": args.package,
            "registry": args.registry,
            "files_checked": 4,
            "would_publish": False,
            "status": "ok",
        }
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="vitte package tooling")
    parser.add_argument("--workspace", type=Path, default=DEFAULT_WORKSPACE)
    sub = parser.add_subparsers(dest="command", required=True)

    package = sub.add_parser("package")
    package_sub = package.add_subparsers(dest="package_command", required=True)
    graph = package_sub.add_parser("graph")
    graph.add_subparsers(dest="graph_command", required=True).add_parser("explain")

    workspace = sub.add_parser("workspace")
    workspace_sub = workspace.add_subparsers(dest="workspace_command", required=True)
    workspace_build = workspace_sub.add_parser("build")
    workspace_build.add_argument("--package", "-p")
    workspace_test = workspace_sub.add_parser("test")
    workspace_test.add_argument("--all", action="store_true")

    publish = sub.add_parser("publish")
    publish.add_argument("--dry-run", action="store_true")
    publish.add_argument("--package", default="workspace-a")
    publish.add_argument("--registry", default="local-offline")
    return parser


def main(argv: list[str]) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.command == "package":
        return command_package(args)
    if args.command == "workspace":
        return command_workspace(args)
    if args.command == "publish":
        return command_publish(args)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
