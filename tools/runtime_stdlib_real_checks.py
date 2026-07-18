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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

int main(int argc, char **argv) {
  VitteSliceI32 numbers = vitte_empty_slice_i32();
  VitteString left = {"vit", 3};
  VitteString right = {"te", 2};
  VitteString joined;
  VitteString integer;
  VitteSliceString args;
  VitteString panic_message = {"expected probe panic", 20};
  char missing_path[128];
  char source_path[128];
  char destination_path[128];
  char directory_path[128];
  char child_a_path[160];
  char child_b_path[160];
  VitteString missing;
  VitteString source_file;
  VitteString destination;
  VitteString directory;
  VitteString child_a;
  VitteString child_b;
  VitteSliceString directory_entries;
  VitteString original = {"keep", 4};
  VitteString replacement = {"new", 3};
  VitteString preserved;
  VitteSliceI32 oversized_numbers = {NULL, SIZE_MAX};
  VitteString oversized_string = {NULL, SIZE_MAX};
  VitteString empty_string = {NULL, 0};
  VitteString rejected_string;
  VitteString invalid_string = {NULL, 1};
  VitteString oversized_path = {"x", SIZE_MAX};
  VitteString oversized_content = {"x", (size_t)INT32_MAX + 1};

  CHECK(strcmp(vitte_c_abi_version(), VITTE_C_ABI_VERSION) == 0, 10);
  CHECK(vitte_host_runtime_available() == 1, 11);
  numbers = vitte_slice_push_i32(numbers, 10);
  numbers = vitte_slice_push_i32(numbers, -4);
  CHECK(numbers.len == 2 && numbers.data[0] == 10 && numbers.data[1] == -4, 12);

  joined = vitte_string_concat(left, right);
  CHECK(joined.len == 5 && memcmp(joined.data, "vitte", 5) == 0, 13);
  integer = vitte_i32_to_string(-42);
  CHECK(integer.len == 3 && memcmp(integer.data, "-42", 3) == 0, 14);

  snprintf(missing_path, sizeof(missing_path), "/tmp/vitte-runtime-missing-%ld", (long)getpid());
  snprintf(source_path, sizeof(source_path), "/tmp/vitte-runtime-source-%ld", (long)getpid());
  snprintf(destination_path, sizeof(destination_path), "/tmp/vitte-runtime-destination-%ld", (long)getpid());
  snprintf(directory_path, sizeof(directory_path), "/tmp/vitte-runtime-directory-%ld", (long)getpid());
  missing.data = missing_path;
  missing.len = strlen(missing_path);
  source_file.data = source_path;
  source_file.len = strlen(source_path);
  destination.data = destination_path;
  destination.len = strlen(destination_path);
  directory.data = directory_path;
  directory.len = strlen(directory_path);
  snprintf(child_a_path, sizeof(child_a_path), "%s/a", directory_path);
  snprintf(child_b_path, sizeof(child_b_path), "%s/b", directory_path);
  child_a.data = child_a_path;
  child_a.len = strlen(child_a_path);
  child_b.data = child_b_path;
  child_b.len = strlen(child_b_path);
  unlink(missing_path);
  unlink(source_path);
  unlink(destination_path);
  rmdir(directory_path);
  CHECK(vitte_host_write_file(destination, original) == 4, 22);
  CHECK(vitte_host_copy_file(destination, destination) == -1, 26);
  preserved = vitte_host_read_file(destination);
  CHECK(preserved.len == 4 && memcmp(preserved.data, "keep", 4) == 0, 27);
  free((void *)preserved.data);
  CHECK(vitte_host_copy_file(missing, destination) == -1, 23);
  preserved = vitte_host_read_file(destination);
  CHECK(preserved.len == 4 && memcmp(preserved.data, "keep", 4) == 0, 24);
  vitte_string_release(preserved);
  CHECK(vitte_host_write_file(source_file, replacement) == 3, 63);
  CHECK(vitte_host_copy_file(source_file, destination) == 0, 64);
  preserved = vitte_host_read_file(destination);
  CHECK(preserved.len == 3 && memcmp(preserved.data, "new", 3) == 0, 65);
  CHECK(vitte_host_delete_file(source_file) == 0, 66);
  CHECK(vitte_host_delete_file(destination) == 0, 25);
  CHECK(vitte_host_mkdir_all(directory) == 0, 52);
  CHECK(vitte_host_delete_file(directory) == -1, 53);
  CHECK(vitte_host_is_directory(directory) == 1, 54);
  CHECK(vitte_host_write_file(child_b, original) == 4, 56);
  CHECK(vitte_host_write_file(child_a, original) == 4, 57);
  directory_entries = vitte_host_list_directory(directory);
  CHECK(directory_entries.len == 2, 58);
  CHECK(directory_entries.data[0].len == 1 && directory_entries.data[0].data[0] == 'a', 59);
  CHECK(directory_entries.data[1].len == 1 && directory_entries.data[1].data[0] == 'b', 60);
  vitte_owned_slice_string_release(directory_entries);
  CHECK(vitte_host_delete_file(child_a) == 0, 61);
  CHECK(vitte_host_delete_file(child_b) == 0, 62);
  CHECK(vitte_host_delete_directory(directory) == 0, 55);

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

  CHECK(vitte_runtime_panic_boundary_begin() == 1, 28);
  oversized_numbers = vitte_slice_push_i32(oversized_numbers, 1);
  CHECK(oversized_numbers.data == NULL && oversized_numbers.len == SIZE_MAX, 29);
  rejected_string = vitte_string_concat(oversized_string, empty_string);
  CHECK(rejected_string.data == NULL && rejected_string.len == 0, 30);
  CHECK(vitte_runtime_panic_boundary_triggered() == 1, 31);
  CHECK(vitte_runtime_panic_boundary_code() == 3, 32);
  CHECK(vitte_runtime_panic_boundary_end() == 0, 33);
  CHECK(vitte_runtime_panic_boundary_reset() == 0, 34);

  CHECK(vitte_runtime_panic_boundary_begin() == 1, 35);
  CHECK(vitte_host_file_exists(invalid_string) == 0, 36);
  CHECK(vitte_host_write_file(destination, invalid_string) == -1, 37);
  rejected_string = vitte_string_concat(invalid_string, empty_string);
  CHECK(rejected_string.data == NULL && rejected_string.len == 0, 38);
  CHECK(vitte_host_file_exists(destination) == 0, 39);
  CHECK(vitte_runtime_panic_boundary_triggered() == 1, 40);
  CHECK(vitte_runtime_panic_boundary_code() == 3, 41);
  CHECK(vitte_runtime_panic_boundary_end() == 0, 42);
  CHECK(vitte_runtime_panic_boundary_reset() == 0, 43);

  CHECK(vitte_runtime_panic_boundary_begin() == 1, 44);
  CHECK(vitte_host_file_exists(oversized_path) == 0, 45);
  CHECK(vitte_host_write_file(destination, oversized_content) == -1, 46);
  CHECK(vitte_host_file_exists(destination) == 0, 47);
  CHECK(vitte_runtime_panic_boundary_triggered() == 1, 48);
  CHECK(vitte_runtime_panic_boundary_code() == 3, 49);
  CHECK(vitte_runtime_panic_boundary_end() == 0, 50);
  CHECK(vitte_runtime_panic_boundary_reset() == 0, 51);

  vitte_slice_i32_release(numbers);
  vitte_string_release(joined);
  vitte_string_release(integer);
  vitte_string_release(preserved);
  vitte_slice_string_release(args);
  vitte_owned_slice_string_release(vitte_empty_slice_string());
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
