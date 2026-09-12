#!/usr/bin/env python3
"""Run one build in an owned process group with bounded memory, time and logs."""
from __future__ import annotations

import argparse
import fcntl
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import threading
import time


DEFAULT_RSS_LIMIT_KIB = 4_000_000
DEFAULT_TIMEOUT_SECONDS = 1_800
POLL_SECONDS = 0.1
RSS_BURST_GRACE_SECONDS = 3.0
RSS_HARD_HEADROOM_MIN_KIB = 8 * 1024
RSS_HARD_HEADROOM_DIVISOR = 20
OUTPUT_TAIL_BYTES = 12_000
MAX_LOG_BYTES = 64 * 1024 * 1024
COMPILER_BUILD_LOCK = Path(__file__).resolve().parents[2] / "target/bootstrap-real/compiler-build.lock"


class GuardInterrupted(BaseException):
    def __init__(self, signum: int):
        self.signum = signum


def interrupt_build(signum: int, _frame) -> None:
    raise GuardInterrupted(signum)


def process_snapshot() -> list[tuple[int, int, int, int]]:
    # POSIX ps reports RSS in KiB on both macOS and Linux. Fail closed if process
    # inspection is forbidden; RLIMIT_RSS alone is not enforced on these hosts.
    result = subprocess.run(
        ["ps", "-axo", "pid=,ppid=,pgid=,rss="], check=True, capture_output=True,
        text=True, timeout=2,
    )
    rows = []
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) != 4:
            raise ValueError("invalid ps memory row")
        rows.append(tuple(int(value) for value in fields))
    if not rows:
        raise ValueError("empty process snapshot")
    return rows


def owned_processes(rows: list[tuple[int, int, int, int]], groups: set[int]) -> set[int]:
    owned = {pid for pid, _parent, pgid, _rss in rows if pgid in groups}
    while True:
        children = {pid for pid, parent, _pgid, _rss in rows if parent in owned}
        if children <= owned:
            break
        owned.update(children)
    # Nested guarded builds start their own sessions. Keep tracking these groups
    # even if their original parent exits; an outer timeout must stop them too.
    groups.update(pid for pid, _parent, pgid, _rss in rows if pid in owned and pid == pgid)
    return owned


def tree_rss_kib(rows: list[tuple[int, int, int, int]], groups: set[int]) -> int:
    owned = owned_processes(rows, groups)
    return sum(rss for pid, _parent, _pgid, rss in rows if pid in owned)


def signal_group(process: subprocess.Popen, group: int, signum: int) -> None:
    try:
        os.killpg(group, signum)
        return
    except PermissionError:
        # Some macOS sandboxes forbid group signals but permit signaling the
        # exact child PIDs. Resolve membership afresh; never use name matching.
        try:
            members = [pid for pid, _parent, pgid, _rss in process_snapshot() if pgid == group]
        except (OSError, ValueError, subprocess.SubprocessError):
            # At least reap our direct child if process inspection has failed.
            members = [process.pid] if process.poll() is None else []
        for pid in sorted(members, key=lambda pid: pid == process.pid):
            try:
                os.kill(pid, signum)
            except ProcessLookupError:
                pass


def stop_group(process: subprocess.Popen, groups: set[int]) -> None:
    # Never signal a caller's group or search by process name. The PID is the
    # unique group we created with start_new_session, including clang children.
    try:
        owned_processes(process_snapshot(), groups)
    except (OSError, ValueError, subprocess.SubprocessError):
        pass
    for group in sorted(groups, key=lambda group: group == process.pid):
        try:
            signal_group(process, group, signal.SIGTERM)
        except ProcessLookupError:
            pass
    time.sleep(0.2)
    for group in sorted(groups, key=lambda group: group == process.pid):
        try:
            signal_group(process, group, signal.SIGKILL)
        except ProcessLookupError:
            pass
    process.wait()


def run_guarded(
    command: list[str], *, cwd: Path, env: dict[str, str] | None = None,
    rss_limit_kib: int = DEFAULT_RSS_LIMIT_KIB,
    timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS,
) -> dict[str, object]:
    if rss_limit_kib <= 0 or timeout_seconds <= 0:
        raise ValueError("resource limits must be positive")
    compiler_build = "build" in command and any(
        arg.replace("\\", "/").endswith("src/vitte/compiler/main.vit") for arg in command
    )
    if not compiler_build:
        return _run_guarded(command, cwd=cwd, env=env, rss_limit_kib=rss_limit_kib,
                            timeout_seconds=timeout_seconds)
    # All compiler stages share intermediate LLVM files. Keep one stable lock
    # inode; closing the handle releases the lock even after interruption.
    COMPILER_BUILD_LOCK.parent.mkdir(parents=True, exist_ok=True)
    with COMPILER_BUILD_LOCK.open("a") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            return {
                "command": command, "exit_code": 75,
                "output": "[resource-guard] stopped: concurrent-compiler-build; another compiler build owns the shared intermediates",
                "resource_guard": {
                    "rss_limit_kib": rss_limit_kib, "timeout_seconds": timeout_seconds,
                    "peak_group_rss_kib": 0, "reason": "concurrent-compiler-build",
                    "elapsed_seconds": 0,
                },
            }
        return _run_guarded(command, cwd=cwd, env=env, rss_limit_kib=rss_limit_kib,
                            timeout_seconds=timeout_seconds)


def _run_guarded(
    command: list[str], *, cwd: Path, env: dict[str, str] | None = None,
    rss_limit_kib: int = DEFAULT_RSS_LIMIT_KIB,
    timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS,
) -> dict[str, object]:
    if rss_limit_kib <= 0 or timeout_seconds <= 0:
        raise ValueError("resource limits must be positive")
    started = time.monotonic()
    result: dict[str, object] = {
        "command": command, "exit_code": 126, "output": "",
        "resource_guard": {
            "rss_limit_kib": rss_limit_kib, "timeout_seconds": timeout_seconds,
            "peak_group_rss_kib": 0, "reason": "", "elapsed_seconds": 0,
        },
    }
    guard = result["resource_guard"]
    hard_rss_limit_kib = rss_limit_kib + max(
        RSS_HARD_HEADROOM_MIN_KIB, rss_limit_kib // RSS_HARD_HEADROOM_DIVISOR,
    )
    guard["hard_rss_limit_kib"] = hard_rss_limit_kib
    guard["rss_burst_grace_seconds"] = RSS_BURST_GRACE_SECONDS
    process = None
    over_limit_since = None
    handlers = {}
    if threading.current_thread() is threading.main_thread():
        for signum in (signal.SIGTERM, signal.SIGINT):
            handlers[signum] = signal.signal(signum, interrupt_build)
    try:
        process_snapshot()  # Verify visibility before launching any build.
        with tempfile.TemporaryFile() as output:
            process = subprocess.Popen(
                command, cwd=cwd, env=env, stdout=output,
                stderr=subprocess.STDOUT, start_new_session=True,
            )
            groups = {process.pid}
            try:
                while True:
                    rss = tree_rss_kib(process_snapshot(), groups)
                    guard["peak_group_rss_kib"] = max(guard["peak_group_rss_kib"], rss)
                    if rss > hard_rss_limit_kib:
                        guard["reason"] = "memory-limit"
                        result["exit_code"] = 137
                        break
                    if rss > rss_limit_kib:
                        if over_limit_since is None:
                            over_limit_since = time.monotonic()
                        elif time.monotonic() - over_limit_since > RSS_BURST_GRACE_SECONDS:
                            guard["reason"] = "memory-limit"
                            result["exit_code"] = 137
                            break
                    else:
                        over_limit_since = None
                    if time.monotonic() - started > timeout_seconds:
                        guard["reason"] = "timeout"
                        result["exit_code"] = 124
                        break
                    if os.fstat(output.fileno()).st_size > MAX_LOG_BYTES:
                        guard["reason"] = "output-limit"
                        result["exit_code"] = 125
                        break
                    status = process.poll()
                    if status is not None:
                        result["exit_code"] = status
                        break
                    time.sleep(POLL_SECONDS)
            finally:
                stop_group(process, groups)
                length = output.seek(0, os.SEEK_END)
                output.seek(max(0, length - OUTPUT_TAIL_BYTES))
                result["output"] = output.read(OUTPUT_TAIL_BYTES).decode("utf-8", errors="replace")
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        guard["reason"] = "monitor-unavailable" if process is None else "monitor-error"
        result["exit_code"] = 126
        result["output"] += f"\n[resource-guard] {exc}"
    except GuardInterrupted as exc:
        guard["reason"] = "interrupted"
        result["exit_code"] = 128 + exc.signum
    finally:
        for signum, handler in handlers.items():
            signal.signal(signum, handler)
    guard["elapsed_seconds"] = round(time.monotonic() - started, 3)
    if guard["reason"]:
        result["output"] += f"\n[resource-guard] stopped: {guard['reason']}"
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rss-limit-kib", type=int, default=DEFAULT_RSS_LIMIT_KIB)
    parser.add_argument("--timeout-seconds", type=float, default=DEFAULT_TIMEOUT_SECONDS)
    parser.add_argument("--report", type=Path)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or args.rss_limit_kib <= 0 or args.timeout_seconds <= 0:
        parser.error("a command and positive resource limits are required")
    result = run_guarded(
        command, cwd=Path.cwd(), rss_limit_kib=args.rss_limit_kib,
        timeout_seconds=args.timeout_seconds,
    )
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(result["output"])
    print("[resource-guard] " + json.dumps(result["resource_guard"], sort_keys=True))
    status = int(result["exit_code"])
    return status if status >= 0 else 128 - status


if __name__ == "__main__":
    raise SystemExit(main())
