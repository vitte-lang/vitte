#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import hmac
import http.client
import json
import socket
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target" / "runtime_stdlib_real"
REPORT = ROOT / "target" / "reports" / "runtime_stdlib_real.md"


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
