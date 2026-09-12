#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import fcntl
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import resource_guard as guard


class ResourceGuardTests(unittest.TestCase):
    def test_compiler_build_lock_rejects_overlap_and_releases(self):
        command = [sys.executable, "-c", "pass", "build", "src/vitte/compiler/main.vit"]
        with tempfile.TemporaryDirectory() as directory:
            lock_path = Path(directory) / "compiler-build.lock"
            with patch.object(guard, "COMPILER_BUILD_LOCK", lock_path):
                with lock_path.open("a") as held:
                    fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    with patch.object(guard.subprocess, "Popen") as launch:
                        result = guard.run_guarded(command, cwd=Path.cwd())
                    launch.assert_not_called()
                    self.assertEqual(result["exit_code"], 75)
                    self.assertEqual(result["resource_guard"]["reason"], "concurrent-compiler-build")
                for _ in range(2):
                    self.assertEqual(guard.run_guarded(command, cwd=Path.cwd())["exit_code"], 0)

    def run_child(self, source: str, **limits):
        return guard.run_guarded([sys.executable, "-c", source], cwd=Path.cwd(), **limits)

    def test_combines_children_and_excludes_other_groups(self):
        self.assertEqual(guard.tree_rss_kib([(10, 1, 10, 4), (11, 10, 10, 6), (12, 11, 10, 8), (20, 1, 20, 100)], {10}), 18)

    def test_tracks_nested_sessions_after_parent_exit(self):
        groups = {10}
        self.assertEqual(guard.tree_rss_kib([(10, 1, 10, 4), (11, 10, 11, 6), (12, 11, 11, 8), (20, 1, 20, 100)], groups), 18)
        self.assertEqual(groups, {10, 11})
        self.assertEqual(guard.tree_rss_kib([(12, 1, 11, 8), (20, 1, 20, 100)], groups), 8)

    def test_monitor_failure_does_not_launch(self):
        with patch.object(guard, "process_snapshot", side_effect=OSError("denied")), patch.object(guard.subprocess, "Popen") as launch:
            result = self.run_child("raise SystemExit(0)")
        launch.assert_not_called()
        self.assertEqual(result["exit_code"], 126)
        self.assertEqual(result["resource_guard"]["reason"], "monitor-unavailable")

    def test_success_and_nonzero_exit(self):
        result = self.run_child("print('ok'); raise SystemExit(7)")
        self.assertEqual(result["exit_code"], 7)
        self.assertEqual(result["output"].strip(), "ok")
        self.assertEqual(result["resource_guard"]["reason"], "")

    def test_output_is_bounded(self):
        result = self.run_child("print('x' * 50000)")
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(len(result["output"]), guard.OUTPUT_TAIL_BYTES)

    def test_timeout(self):
        result = self.run_child("import time; time.sleep(10)", timeout_seconds=0.4)
        self.assertEqual(result["exit_code"], 124)
        self.assertEqual(result["resource_guard"]["reason"], "timeout")

    def test_external_interruption_cleans_up_build(self):
        result = self.run_child("import os, signal, time; time.sleep(0.2); os.kill(os.getppid(), signal.SIGTERM); time.sleep(10)")
        self.assertEqual(result["exit_code"], 143)
        self.assertEqual(result["resource_guard"]["reason"], "interrupted")
        self.assertLess(result["resource_guard"]["elapsed_seconds"], 5)

    def test_monitor_failure_after_launch_stops_child(self):
        real_snapshot = guard.process_snapshot
        calls = 0
        def failing_once():
            nonlocal calls
            calls += 1
            if calls == 2:
                raise OSError("denied")
            return real_snapshot()
        with patch.object(guard, "process_snapshot", side_effect=failing_once):
            result = self.run_child("import time; time.sleep(10)")
        self.assertEqual(result["exit_code"], 126)
        self.assertEqual(result["resource_guard"]["reason"], "monitor-error")
        self.assertLess(result["resource_guard"]["elapsed_seconds"], 5)

    def test_memory_limit_includes_compiler_child(self):
        # A small, bounded allocation in a child, not in the monitored leader.
        source = "import subprocess, sys; p = subprocess.Popen([sys.executable, '-c', 'import time; x = bytearray(48 * 1024 * 1024); time.sleep(10)']); print(p.pid, flush=True); p.wait()"
        result = self.run_child(source, rss_limit_kib=45_000, timeout_seconds=5)
        self.assertEqual(result["exit_code"], 137)
        self.assertEqual(result["resource_guard"]["reason"], "memory-limit")
        self.assertGreater(result["resource_guard"]["peak_group_rss_kib"], 45_000)
        child = int(result["output"].splitlines()[0])
        state = subprocess.run(["ps", "-o", "stat=", "-p", str(child)], capture_output=True, text=True)
        self.assertTrue(not state.stdout.strip() or state.stdout.strip().startswith("Z"))

    def test_brief_soft_limit_burst_is_tolerated(self):
        source = "import time; x = bytearray(16 * 1024 * 1024); time.sleep(0.2); del x"
        with patch.object(guard, "RSS_BURST_GRACE_SECONDS", 1.0), \
             patch.object(guard, "RSS_HARD_HEADROOM_MIN_KIB", 100_000):
            result = self.run_child(source, rss_limit_kib=20_000, timeout_seconds=3)
        self.assertEqual(result["exit_code"], 0)
        self.assertEqual(result["resource_guard"]["reason"], "")
        self.assertGreater(result["resource_guard"]["peak_group_rss_kib"], 20_000)

    def test_outer_timeout_stops_nested_session(self):
        source = "import subprocess, sys; p = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(10)'], start_new_session=True); print(p.pid, flush=True); p.wait()"
        result = self.run_child(source, timeout_seconds=0.5)
        self.assertEqual(result["exit_code"], 124)
        child = int(result["output"].splitlines()[0])
        state = subprocess.run(["ps", "-o", "stat=", "-p", str(child)], capture_output=True, text=True)
        self.assertTrue(not state.stdout.strip() or state.stdout.strip().startswith("Z"))

    def test_invalid_limits(self):
        with self.assertRaises(ValueError):
            self.run_child("pass", rss_limit_kib=0)


if __name__ == "__main__":
    unittest.main()
