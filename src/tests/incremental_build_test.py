#!/usr/bin/env python3
"""Exercise Make's source, module, compiler, and flag invalidation paths."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAKEFILE = ROOT / "Makefile"


def run_make(target: Path, assignments: list[str], env: dict[str, str]) -> str:
    result = subprocess.run(
        ["make", "--no-print-directory", "-f", str(MAKEFILE), str(target), *assignments],
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if result.returncode != 0:
        raise AssertionError(f"make failed ({result.returncode}):\n{result.stdout}")
    return result.stdout


def write_fake_compiler(path: Path, counter: Path, kind: str) -> None:
    source = f"""#!/usr/bin/env python3
import os
import sys
from pathlib import Path

args = sys.argv[1:]
output = Path(args[args.index('-o') + 1])
output.parent.mkdir(parents=True, exist_ok=True)
counter = Path(os.environ['FAKE_COMPILER_COUNT'])
counter.parent.mkdir(parents=True, exist_ok=True)
with counter.open('a', encoding='utf-8') as stream:
    stream.write('run\\n')
output.write_text({kind!r} + '\\n', encoding='utf-8')
if '-MF' in args and os.environ.get('FAKE_COMPILER_HEADER'):
    source = args[args.index('-c') + 1]
    depfile = Path(args[args.index('-MF') + 1])
    depfile.parent.mkdir(parents=True, exist_ok=True)
    depfile.write_text(
        f"{{output}}: {{source}} {{os.environ['FAKE_COMPILER_HEADER']}}\\n",
        encoding='utf-8',
    )
"""
    path.write_text(source, encoding="utf-8")
    path.chmod(0o755)


def invocation_count(counter: Path) -> int:
    if not counter.exists():
        return 0
    return len(counter.read_text(encoding="utf-8").splitlines())


def check_object_configuration_invalidation(temp: Path) -> None:
    fake_cc = temp / "fake-cc"
    counter = temp / "cc-count"
    write_fake_compiler(fake_cc, counter, "object")
    build = temp / "object-build"
    target = build / "obj" / "main.o"
    header = temp / "build-invalidation.h"
    header.write_text("/* tracked header */\n", encoding="utf-8")
    env = os.environ.copy()
    env["FAKE_COMPILER_COUNT"] = str(counter)
    env["FAKE_COMPILER_HEADER"] = str(header)

    def invoke(mode: str, flags: str) -> None:
        run_make(
            target,
            [
                f"BUILD={build}",
                f"MODE={mode}",
                f"CC={fake_cc}",
                "CPPFLAGS=",
                f"CFLAGS={flags}",
                "LDFLAGS=",
                "LDLIBS=",
            ],
            env,
        )

    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 1, "initial object should compile once"
    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 1, "unchanged configuration should be a no-op"
    future = target.stat().st_mtime_ns + 3_000_000_000
    os.utime(header, ns=(future, future))
    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 2, "changed header dependency must rebuild the object"
    older = target.stat().st_mtime_ns - 1_000_000
    os.utime(header, ns=(older, older))
    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 2, "settled header timestamp should remain incremental"
    invoke("debug", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 3, "changed CFLAGS must rebuild the object"
    invoke("release", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 4, "changed build profile must rebuild the object"
    invoke("release", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 4, "unchanged release configuration should be a no-op"


def check_link_configuration_invalidation(temp: Path) -> None:
    fake_cc = temp / "fake-link-cc"
    counter = temp / "link-count"
    write_fake_compiler(fake_cc, counter, "linked compiler")
    build = temp / "link-build"
    target = build / "bin" / "vitte-bootstrap"
    env = os.environ.copy()
    env["FAKE_COMPILER_COUNT"] = str(counter)

    def invoke(mode: str, flags: str) -> None:
        run_make(
            target,
            [
                f"BUILD={build}",
                f"MODE={mode}",
                "SOURCES=src/bootstrap/main.c",
                f"CC={fake_cc}",
                "CPPFLAGS=",
                f"CFLAGS={flags}",
                "LDFLAGS=",
                "LDLIBS=",
            ],
            env,
        )

    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 2, "initial link should compile and link once"
    invoke("debug", "-DTEST_CONFIGURATION=one")
    assert invocation_count(counter) == 2, "unchanged compiler build should be a no-op"
    invoke("debug", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 4, "changed CFLAGS must rebuild and relink"
    invoke("release", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 6, "changed profile must rebuild and relink"
    invoke("release", "-DTEST_CONFIGURATION=two")
    assert invocation_count(counter) == 6, "unchanged release compiler should be a no-op"


def check_native_frontend_invalidation(temp: Path) -> None:
    counter = temp / "frontend-count"
    fake_compiler = temp / "fake-vitte"
    write_fake_compiler(fake_compiler, counter, "generated C")
    source = temp / "api.vitte"
    manifest = temp / "Vitte.toml"
    module = temp / "module.vit"
    source.write_text("module api\n", encoding="utf-8")
    manifest.write_text("name = 'fixture'\n", encoding="utf-8")
    module.write_text("module fixture\n", encoding="utf-8")

    build = temp / "frontend-build"
    output = build / "native-frontend" / "api-implementation.c"
    env = os.environ.copy()
    env["FAKE_COMPILER_COUNT"] = str(counter)
    assignments = [
        f"BUILD={build}",
        f"NATIVE_FRONTEND_DIR={output.parent}",
        f"NATIVE_FRONTEND_C={output}",
        f"NATIVE_FRONTEND_COMPILER={fake_compiler}",
        f"NATIVE_FRONTEND_ENTRY={source}",
        f"NATIVE_FRONTEND_SOURCES={source}",
        f"NATIVE_MODULE_SOURCES={manifest} {module}",
        f"VITTE_MODULE_PATH={temp}",
    ]

    def invoke() -> None:
        run_make(output, assignments, env)

    invoke()
    assert invocation_count(counter) == 1, "initial frontend generation should run once"
    invoke()
    assert invocation_count(counter) == 1, "unchanged frontend inputs should be a no-op"

    prerequisites = [fake_compiler, source, manifest, module]
    for expected, prerequisite in enumerate(prerequisites, start=2):
        future = output.stat().st_mtime_ns + 3_000_000_000
        os.utime(prerequisite, ns=(future, future))
        invoke()
        assert invocation_count(counter) == expected, (
            f"changing {prerequisite.name} should regenerate frontend C"
        )

        # Return the touched prerequisite behind the just-written output so
        # each following assertion measures only the next dependency.
        older = output.stat().st_mtime_ns - 1_000_000
        os.utime(prerequisite, ns=(older, older))
        invoke()
        assert invocation_count(counter) == expected, (
            f"settled {prerequisite.name} timestamp should remain incremental"
        )


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="vitte-incremental-") as directory:
        temp = Path(directory)
        check_object_configuration_invalidation(temp)
        check_link_configuration_invalidation(temp)
        check_native_frontend_invalidation(temp)
    print("incremental build invalidation passed")


if __name__ == "__main__":
    main()
