#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "bootstrap" / "offline" / "report.json"
CLEAN_REPORT = ROOT / "target" / "bootstrap" / "clean-checkout" / "report.json"
NETWORK_PROBE = """
import errno
import socket
import sys

blocked = {errno.EACCES, errno.EPERM, errno.ENETDOWN, errno.ENETUNREACH, errno.EHOSTUNREACH}
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(2.0)
try:
    sock.connect(("198.51.100.1", 9))
except OSError as error:
    print(error.errno if error.errno is not None else -1)
    raise SystemExit(0 if error.errno in blocked else 2)
finally:
    sock.close()
raise SystemExit(1)
""".strip()


def command_result(command: list[str], timeout: int = 30, environment: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=ROOT,
            env=environment,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout if isinstance(error.stdout, str) else ""
        stderr = error.stderr if isinstance(error.stderr, str) else ""
        return subprocess.CompletedProcess(command, 124, stdout, stderr or "command timed out")
    except OSError as error:
        return subprocess.CompletedProcess(command, 127, "", str(error))


def linux_prefix_candidates() -> list[tuple[str, list[str]]]:
    candidates: list[tuple[str, list[str]]] = []
    unshare = shutil.which("unshare")
    if unshare:
        candidates.append(
            (
                "linux-user-network-namespace",
                [unshare, "--user", "--map-root-user", "--net", "--"],
            )
        )
        sudo = shutil.which("sudo")
        if sudo:
            user_name = os.environ.get("USER", "")
            if user_name:
                candidates.append(
                    (
                        "linux-sudo-network-namespace",
                        [sudo, "-n", unshare, "--net", "--", sudo, "-n", "-u", user_name, "--"],
                    )
                )
    bubblewrap = shutil.which("bwrap")
    if bubblewrap:
        candidates.append(
            (
                "linux-bubblewrap-network-namespace",
                [bubblewrap, "--unshare-net", "--dev-bind", "/", "/", "--"],
            )
        )
    return candidates


def isolation_candidates() -> list[tuple[str, list[str]]]:
    system = platform.system()
    if system == "Darwin":
        sandbox = shutil.which("sandbox-exec")
        if not sandbox:
            return []
        profile = "(version 1) (allow default) (deny network*)"
        return [("macos-sandbox-network-deny", [sandbox, "-p", profile])]
    if system == "Linux":
        return linux_prefix_candidates()
    return []


def select_isolation() -> tuple[
    tuple[str, list[str], subprocess.CompletedProcess[str]] | None,
    list[dict[str, object]],
]:
    attempts: list[dict[str, object]] = []
    for mechanism, prefix in isolation_candidates():
        probe = command_result(prefix + [sys.executable, "-c", NETWORK_PROBE])
        attempts.append(
            {
                "mechanism": mechanism,
                "returncode": probe.returncode,
                "stdout": output_tail(probe.stdout),
                "stderr": output_tail(probe.stderr),
            }
        )
        if probe.returncode == 0:
            return (mechanism, prefix, probe), attempts
    return None, attempts


def offline_environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment.update(
        {
            "VITTE_OFFLINE": "1",
            "GIT_TERMINAL_PROMPT": "0",
            "PIP_NO_INDEX": "1",
            "npm_config_offline": "true",
            "CARGO_NET_OFFLINE": "true",
            "GOPROXY": "off",
            "http_proxy": "http://127.0.0.1:9",
            "https_proxy": "http://127.0.0.1:9",
            "HTTP_PROXY": "http://127.0.0.1:9",
            "HTTPS_PROXY": "http://127.0.0.1:9",
            "NO_PROXY": "",
            "no_proxy": "",
        }
    )
    return environment


def output_tail(text: str) -> str:
    return "\n".join(text.splitlines()[-80:])


def write_report(payload: dict[str, object]) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    temporary = REPORT.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(REPORT)


def main() -> int:
    parser = argparse.ArgumentParser(description="verify a clean bootstrap with kernel-enforced network isolation")
    parser.parse_args()

    payload: dict[str, object] = {
        "schema": "vitte.bootstrap.offline",
        "schema_version": "1.0.0",
        "platform": platform.system(),
        "status": "fail",
        "mechanism": "unavailable",
        "network_probe": {"ok": False, "returncode": 127},
        "bootstrap": {"ok": False, "returncode": 127},
    }
    selected, attempts = select_isolation()
    payload["isolation_attempts"] = attempts
    if selected is None:
        payload["message"] = "no verified kernel network isolation mechanism is available"
        write_report(payload)
        print(f"[bootstrap-offline][error] {payload['message']}", file=sys.stderr)
        return 2

    mechanism, prefix, probe = selected
    payload["mechanism"] = mechanism
    payload["network_probe"] = {
        "ok": True,
        "returncode": probe.returncode,
        "stdout": output_tail(probe.stdout),
        "stderr": output_tail(probe.stderr),
    }
    CLEAN_REPORT.unlink(missing_ok=True)
    command = prefix + [sys.executable, "tools/bootstrap_clean_checkout.py"]
    completed = command_result(command, timeout=360, environment=offline_environment())
    clean_payload: dict[str, object] = {}
    if CLEAN_REPORT.is_file():
        clean_payload = json.loads(CLEAN_REPORT.read_text(encoding="utf-8"))
    clean_ok = clean_payload.get("status") == "ok"
    bootstrap_ok = completed.returncode == 0 and clean_ok
    payload["bootstrap"] = {
        "ok": bootstrap_ok,
        "returncode": completed.returncode,
        "clean_checkout_status": clean_payload.get("status", "missing"),
        "stdout_tail": output_tail(completed.stdout),
        "stderr_tail": output_tail(completed.stderr),
    }
    payload["status"] = "ok" if bootstrap_ok else "fail"
    write_report(payload)

    if bootstrap_ok:
        print(f"[bootstrap-offline] ok: mechanism={mechanism} report={REPORT.relative_to(ROOT)}")
        return 0
    print(f"[bootstrap-offline][error] bootstrap failed: report={REPORT.relative_to(ROOT)}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
