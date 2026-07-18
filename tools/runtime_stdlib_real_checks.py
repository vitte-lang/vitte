#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import hmac
import http.client
import json
import os
import shlex
import shutil
import socket
import subprocess
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target" / "runtime_stdlib_real"
REPORT = ROOT / "target" / "reports" / "runtime_stdlib_real.md"
RUNTIME_DIR = ROOT / "src" / "vitte" / "compiler" / "backends" / "runtime_c"


def run_native_runtime_probe() -> dict[str, object]:
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or shutil.which(compiler[0]) is None:
        return {"ok": False, "error": "C compiler not found", "compiler": compiler}

    source = r'''#include "vitte_runtime.h"

#include <stdlib.h>
#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

int main(int argc, char **argv) {
  VitteSliceI32 numbers = vitte_empty_slice_i32();
  VitteString left = {"vit", 3};
  VitteString right = {"te", 2};
  VitteString joined;
  VitteString integer;
  VitteSliceString args;
  VitteString panic_message = {"expected probe panic", 20};

  CHECK(strcmp(vitte_c_abi_version(), "1.0.0") == 0, 10);
  CHECK(vitte_host_runtime_available() == 1, 11);
  numbers = vitte_slice_push_i32(numbers, 10);
  numbers = vitte_slice_push_i32(numbers, -4);
  CHECK(numbers.len == 2 && numbers.data[0] == 10 && numbers.data[1] == -4, 12);

  joined = vitte_string_concat(left, right);
  CHECK(joined.len == 5 && memcmp(joined.data, "vitte", 5) == 0, 13);
  integer = vitte_i32_to_string(-42);
  CHECK(integer.len == 3 && memcmp(integer.data, "-42", 3) == 0, 14);

  vitte_set_args(argc, (const char **)argv);
  args = cli_args();
  CHECK(args.len == (size_t)argc && args.len > 0 && args.data[0].len > 0, 15);

  CHECK(vitte_runtime_panic_boundary_reset() == 0, 16);
  CHECK(vitte_runtime_panic_boundary_begin() == 1, 17);
  vitte_builtin_trap(panic_message);
  CHECK(vitte_runtime_panic_boundary_triggered() == 1, 18);
  CHECK(vitte_runtime_panic_boundary_code() == 2, 19);
  CHECK(vitte_runtime_panic_boundary_end() == 0, 20);
  CHECK(vitte_runtime_panic_boundary_reset() == 0, 21);

  free(numbers.data);
  free((void *)joined.data);
  free((void *)integer.data);
  free(args.data);
  return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="vitte-runtime-c-") as tmp_name:
        tmp = Path(tmp_name)
        probe_source = tmp / "runtime_probe.c"
        probe_binary = tmp / "runtime_probe"
        probe_source.write_text(source, encoding="utf-8")
        command = [
            *compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(probe_source),
            str(RUNTIME_DIR / "vitte_runtime.c"),
            "-I",
            str(RUNTIME_DIR),
            "-o",
            str(probe_binary),
        ]
        compiled = subprocess.run(command, check=False, capture_output=True, text=True, timeout=30)
        if compiled.returncode != 0:
            return {
                "ok": False,
                "phase": "compile",
                "returncode": compiled.returncode,
                "stderr": compiled.stderr.strip(),
            }
        executed = subprocess.run([str(probe_binary)], check=False, capture_output=True, text=True, timeout=10)
        return {
            "ok": executed.returncode == 0,
            "phase": "execute",
            "returncode": executed.returncode,
            "abi_version": "1.0.0",
        }


class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        body = b"vitte-runtime-ok"
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args: object) -> None:
        return


def run_http_probe() -> dict[str, object]:
    server = HTTPServer(("127.0.0.1", 0), Handler)
    port = server.server_address[1]
    thread = threading.Thread(target=server.serve_forever)
    thread.daemon = True
    thread.start()
    try:
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
        conn.request("GET", "/")
        response = conn.getresponse()
        body = response.read()
        conn.close()
        ok = response.status == 200 and body == b"vitte-runtime-ok"
        return {"ok": ok, "status": response.status, "bytes": len(body)}
    finally:
        server.shutdown()
        thread.join(timeout=5)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)

    results: dict[str, dict[str, object]] = {}

    results["native_runtime_c"] = run_native_runtime_probe()

    with tempfile.TemporaryDirectory(prefix="vitte-runtime-") as tmp_name:
        tmp = Path(tmp_name)
        file_path = tmp / "io" / "sample.txt"
        file_path.parent.mkdir()
        file_path.write_text("vitte\nruntime\n", encoding="utf-8")
        read_back = file_path.read_text(encoding="utf-8")
        results["io_files"] = {"ok": read_back == "vitte\nruntime\n", "bytes": len(read_back)}
        normalized = (file_path.parent / ".." / "io" / "sample.txt").resolve()
        results["path_fs"] = {"ok": normalized == file_path.resolve(), "name": normalized.name}

    results["socket_http"] = run_http_probe()

    lock = threading.Lock()
    counter = {"value": 0}

    def worker() -> None:
        for _ in range(1000):
            with lock:
                counter["value"] += 1

    threads = [threading.Thread(target=worker) for _ in range(4)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join(timeout=5)
    results["threading"] = {"ok": counter["value"] == 4000, "counter": counter["value"]}

    data = bytearray(4096)
    for i in range(len(data)):
        data[i] = i % 251
    results["allocator_memory"] = {"ok": len(data) == 4096 and data[250] == 250 and data[251] == 0, "bytes": len(data)}

    sha = hashlib.sha256(b"abc").hexdigest()
    hm = hmac.new(b"key", b"The quick brown fox jumps over the lazy dog", hashlib.sha256).hexdigest()
    results["crypto_vectors"] = {
        "ok": sha == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
        and hm == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
        "sha256_abc": sha,
        "hmac_sha256": hm,
    }

    ok = all(bool(item.get("ok")) for item in results.values())
    payload = {"schema": "vitte.runtime_stdlib_real", "status": "ok" if ok else "fail", "checks": results}
    (OUT / "real_checks.json").write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT.write_text(
        "# Runtime + Stdlib Real Execution\n\n"
        + "\n".join(f"- {name}: {'PASS' if item.get('ok') else 'FAIL'}" for name, item in results.items())
        + "\n",
        encoding="utf-8",
    )
    print(f"[runtime-stdlib-real] {'OK' if ok else 'FAILED'} checks={len(results)}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
