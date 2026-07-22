#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]


def run(vitte: Path, args: list[str], environment: dict[str, str], expected: int = 0) -> subprocess.CompletedProcess[str]:
    process = subprocess.run(
        [str(vitte), *args],
        cwd=ROOT,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if process.returncode != expected:
        raise AssertionError(
            f"command returned {process.returncode}, expected {expected}: {vitte} {' '.join(args)}\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )
    return process


def run_json(vitte: Path, args: list[str], environment: dict[str, str], schema: str) -> dict[str, Any]:
    process = run(vitte, args, environment)
    value = json.loads(process.stdout)
    assert value["schema"] == schema, value
    assert value["status"] == "ok", value
    return value


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--vitte", type=Path, default=ROOT / "bin" / "vitte")
    args = parser.parse_args(argv)
    vitte = args.vitte.expanduser().resolve()
    if not vitte.is_file():
        raise AssertionError(f"missing Vitte executable: {vitte}")

    with tempfile.TemporaryDirectory(prefix="vitte-package-cli-") as temporary:
        temp = Path(temporary)
        environment = {
            "HOME": str(temp / "home"),
            "PATH": "/usr/bin:/bin",
            "VITTE_PACKAGE_OFFLINE": "1",
            "http_proxy": "http://127.0.0.1:1",
            "https_proxy": "http://127.0.0.1:1",
            "no_proxy": "*",
        }
        (temp / "home").mkdir()

        standalone = temp / "standalone"
        run_json(vitte, ["package", "init", "standalone", "--path", str(standalone)], environment, "vitte.package.init")
        standalone_manifest = json.loads((standalone / "vitte-package.json").read_text(encoding="utf-8"))
        standalone_manifest["dependencies"] = {"paris": "0.1.0"}
        write_json(standalone / "vitte-package.json", standalone_manifest)
        run_json(vitte, ["package", "check", "--path", str(standalone)], environment, "vitte.package.check")
        built = run_json(vitte, ["package", "build", "--path", str(standalone)], environment, "vitte.package.build")
        binary = Path(built["artifacts"][0])
        assert binary.is_file() and os.access(binary, os.X_OK), binary
        run(binary, [], environment)
        installed = run_json(
            vitte,
            ["package", "install", "--path", str(standalone), "--prefix", str(temp / "installed")],
            environment,
            "vitte.package.install",
        )
        install_root = Path(installed["destination"])
        assert (install_root / "installed.json").is_file()
        run_json(
            vitte,
            ["package", "publish", "--dry-run", "--path", str(standalone)],
            environment,
            "vitte.publish.dry_run",
        )

        workspace = temp / "workspace"
        app = workspace / "packages" / "app"
        lib = workspace / "packages" / "lib"
        run_json(vitte, ["package", "init", "workspace-app", "--path", str(app)], environment, "vitte.package.init")
        run_json(vitte, ["package", "init", "workspace-lib", "--lib", "--path", str(lib)], environment, "vitte.package.init")
        app_manifest = json.loads((app / "vitte-package.json").read_text(encoding="utf-8"))
        app_manifest["dependencies"] = {"workspace-lib": {"source": "workspace", "version": "0.1.0"}}
        write_json(app / "vitte-package.json", app_manifest)
        workspace_file = workspace / "vitte-workspace.json"
        write_json(
            workspace_file,
            {
                "members": ["packages/app", "packages/lib"],
                "registry": {"mode": "offline"},
                "schema": "vitte.workspace.v1",
                "version": "0.1.0",
            },
        )
        graph = run_json(
            vitte,
            ["package", "graph", "--workspace", str(workspace_file)],
            environment,
            "vitte.package.graph",
        )
        assert graph["build_order"] == ["workspace-lib", "workspace-app"], graph
        explained = run_json(
            vitte,
            ["package", "graph", "explain", "--workspace", str(workspace_file)],
            environment,
            "vitte.package.graph.explain",
        )
        assert explained["edge_count"] == 1 and explained["explanation"], explained
        explained_before_subcommand = run_json(
            vitte,
            ["package", "graph", "--workspace", str(workspace_file), "explain"],
            environment,
            "vitte.package.graph.explain",
        )
        assert explained_before_subcommand["workspace"] == str(workspace_file.resolve()), explained_before_subcommand
        workspace_build = run_json(
            vitte,
            ["workspace", "build", "--workspace", str(workspace_file), "--package", "workspace-app"],
            environment,
            "vitte.workspace.build",
        )
        assert workspace_build["build_order"] == ["workspace-lib", "workspace-app"], workspace_build
        workspace_test = run_json(
            vitte,
            ["workspace", "test", "--all", "--workspace", str(workspace_file)],
            environment,
            "vitte.workspace.test",
        )
        assert workspace_test["test_count"] == 2, workspace_test

        broken_manifest = json.loads((standalone / "vitte-package.json").read_text(encoding="utf-8"))
        broken_manifest["dependencies"] = {"missing-offline-package": "9.9.9"}
        write_json(standalone / "vitte-package.json", broken_manifest)
        failure = run(vitte, ["package", "check", "--path", str(standalone)], environment, expected=1)
        assert "PKG_E_DEPENDENCY_NOT_FOUND" in failure.stderr, failure.stderr

    print("[package-cli-integration] ok: init check build run install graph explain publish workspace-build workspace-test offline-error")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
