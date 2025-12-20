#!/usr/bin/env python3
import argparse
import difflib
import os
import subprocess
import sys
import tempfile


def normalize_source(source_path):
    with open(source_path, "rb") as fh:
        data = fh.read()
    normalized = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    if normalized == data:
        return source_path, None

    fd, normalized_path = tempfile.mkstemp(suffix=".vitte")
    os.close(fd)
    with open(normalized_path, "wb") as temp_file:
        temp_file.write(normalized)
    return normalized_path, normalized_path


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="run_cli_tokens.py",
        description="Run `vittec --tokens` and diff the output against the expected file.",
    )
    parser.add_argument("vittec_bin", help="Path to the `vittec` executable")
    parser.add_argument("source", help="Source `.vitte` file to tokenize")
    parser.add_argument("expected", help="Expected tokens output")
    args = parser.parse_args()

    if not os.path.isfile(args.vittec_bin) or not os.access(args.vittec_bin, os.X_OK):
        print(f"error: vittec binary not executable: {args.vittec_bin}", file=sys.stderr)
        return 1

    if not os.path.isfile(args.source):
        print(f"error: missing source file: {args.source}", file=sys.stderr)
        return 1

    if not os.path.isfile(args.expected):
        print(f"error: missing expected output: {args.expected}", file=sys.stderr)
        return 1

    normalized_source, normalized_path = normalize_source(args.source)
    temp_fd, temp_path = tempfile.mkstemp()
    os.close(temp_fd)
    cleanup_paths = [temp_path]
    if normalized_path is not None:
        cleanup_paths.append(normalized_path)

    try:
        with open(temp_path, "w", encoding="utf-8") as tmp_file:
            subprocess.run(
                [args.vittec_bin, "--tokens", normalized_source],
                check=True,
                stdout=tmp_file,
                stderr=subprocess.PIPE,
                text=True,
            )

        with open(args.expected, encoding="utf-8", errors="surrogateescape") as expected_file:
            expected_lines = expected_file.readlines()
        with open(temp_path, encoding="utf-8", errors="surrogateescape") as actual_file:
            actual_lines = actual_file.readlines()

        if expected_lines != actual_lines:
            diff_lines = difflib.unified_diff(
                expected_lines,
                actual_lines,
                fromfile=args.expected,
                tofile=temp_path,
                lineterm="",
            )
            for line in diff_lines:
                print(line)
            return 1
    except subprocess.CalledProcessError as exc:
        print(
            f"error: `{args.vittec_bin} --tokens {args.source}` failed with exit code {exc.returncode}",
            file=sys.stderr,
        )
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        return 1
    finally:
        for path in cleanup_paths:
            try:
                os.remove(path)
            except OSError:
                pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
