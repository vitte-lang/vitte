

#!/usr/bin/env python3
"""validate_json_schema.py

JSON Schema validator utility for Vitte generator/tooling pipelines.

Goals:
- Validate one or many JSON files against a JSON Schema (Draft 2020-12).
- Work well in CI: stable exit codes, useful error output.
- Supports:
  - directory scanning with include/exclude globs
  - reading schema from file
  - reading JSON from file(s)
  - summary report and optional machine-readable JSON report

Exit codes:
- 0: all files valid
- 1: validation failures
- 2: usage / input error
- 3: internal error (unexpected)

Dependencies:
- Optional: `jsonschema` Python package.
  If missing, the tool can fall back to a minimal subset check (structure only) but
  cannot perform full validation.

Examples:
  python3 validate_json_schema.py --schema case.schema.json --in cases/
  python3 validate_json_schema.py --schema case.schema.json --files a.json b.json
  python3 validate_json_schema.py --schema case.schema.json --in cases --include "**/*.case.json" --report out/report.json

"""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import sys
import traceback
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple


@dataclass
class Issue:
    path: str
    message: str
    json_path: str


@dataclass
class Result:
    valid: int = 0
    invalid: int = 0
    errors: int = 0
    issues: List[Issue] = None  # type: ignore

    def __post_init__(self) -> None:
        if self.issues is None:
            self.issues = []


def eprint(*a: object) -> None:
    print(*a, file=sys.stderr)


def split_globs(items: List[str]) -> List[str]:
    out: List[str] = []
    for it in items:
        if not it:
            continue
        for p in it.split(","):
            p = p.strip()
            if p:
                out.append(p)
    return out


def match_any(rel_posix: str, globs: List[str]) -> bool:
    return any(fnmatch.fnmatch(rel_posix, g) for g in globs)


def iter_files(root: Path) -> Iterable[Path]:
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        filenames.sort()
        for fn in filenames:
            yield Path(dirpath) / fn


def load_json(path: Path) -> Any:
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        raise ValueError(f"not found: {path}")
    except json.JSONDecodeError as e:
        raise ValueError(f"invalid JSON: {path}: {e}")


def _json_path_from_error(err: Any) -> str:
    # jsonschema.ValidationError exposes .absolute_path and .json_path
    jp = getattr(err, "json_path", "")
    if isinstance(jp, str) and jp:
        return jp
    ap = getattr(err, "absolute_path", None)
    if ap is None:
        return ""
    try:
        parts = [str(x) for x in ap]
        return "/" + "/".join(parts)
    except Exception:
        return ""


def validate_with_jsonschema(schema: Any, instance: Any) -> List[Tuple[str, str]]:
    """Return list of (json_path, message)"""
    try:
        import jsonschema  # type: ignore
        from jsonschema.validators import validator_for  # type: ignore
    except Exception as e:
        raise RuntimeError("missing dependency: jsonschema") from e

    # Determine appropriate validator (Draft 2020-12 supported in modern jsonschema)
    V = validator_for(schema)
    V.check_schema(schema)
    validator = V(schema)

    issues: List[Tuple[str, str]] = []
    for err in sorted(validator.iter_errors(instance), key=lambda e: str(e.path)):
        issues.append((_json_path_from_error(err), err.message))

    return issues


def minimal_shape_check(schema: Any, instance: Any) -> List[Tuple[str, str]]:
    """Fallback check when jsonschema is unavailable.

    This is intentionally minimal: validates only top-level `type` and `required`.
    It is NOT a full schema validator.
    """

    issues: List[Tuple[str, str]] = []

    if isinstance(schema, dict):
        sch_type = schema.get("type")
        if sch_type == "object" and not isinstance(instance, dict):
            issues.append(("", "expected object"))
        if sch_type == "array" and not isinstance(instance, list):
            issues.append(("", "expected array"))

        req = schema.get("required")
        if isinstance(req, list) and isinstance(instance, dict):
            for k in req:
                if isinstance(k, str) and k not in instance:
                    issues.append(("/" + k, "missing required property"))

    return issues


def pick_inputs(
    *,
    root: Optional[Path],
    files: List[Path],
    include_globs: List[str],
    exclude_globs: List[str],
) -> List[Path]:
    out: List[Path] = []

    if files:
        for p in files:
            if not p.exists() or not p.is_file():
                raise ValueError(f"input is not a file: {p}")
            out.append(p)
        return out

    if root is None:
        raise ValueError("either --files or --in must be provided")

    if not root.exists() or not root.is_dir():
        raise ValueError(f"--in is not a directory: {root}")

    root_posix = root.as_posix().rstrip("/") + "/"

    def rel_posix(p: Path) -> str:
        ap = p.resolve().as_posix()
        if ap.startswith(root_posix):
            return ap[len(root_posix) :]
        return p.as_posix()

    for p in iter_files(root):
        if not p.is_file():
            continue
        rel = rel_posix(p)

        if exclude_globs and match_any(rel, exclude_globs):
            continue

        if include_globs:
            if not match_any(rel, include_globs):
                continue
        else:
            # default: json only
            if p.suffix.lower() != ".json":
                continue

        out.append(p)

    out.sort(key=lambda x: x.as_posix())
    return out


def emit_report_json(res: Result, out_path: Path) -> None:
    payload: Dict[str, Any] = {
        "summary": {
            "valid": res.valid,
            "invalid": res.invalid,
            "errors": res.errors,
            "total": res.valid + res.invalid + res.errors,
        },
        "issues": [
            {
                "file": i.path,
                "json_path": i.json_path,
                "message": i.message,
            }
            for i in res.issues
        ],
    }
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(prog="validate_json_schema.py")

    ap.add_argument("--schema", required=True, help="Path to JSON Schema")

    src = ap.add_argument_group("inputs")
    src.add_argument("--in", dest="in_dir", default="", help="Directory to scan for JSON files")
    src.add_argument("--files", nargs="*", default=[], help="Explicit JSON files to validate")

    flt = ap.add_argument_group("filters")
    flt.add_argument(
        "--include",
        action="append",
        default=[],
        help="Include glob(s) relative to --in (comma-separated allowed). Default: '*.json'",
    )
    flt.add_argument(
        "--exclude",
        action="append",
        default=[],
        help="Exclude glob(s) relative to --in (comma-separated allowed).",
    )

    outg = ap.add_argument_group("output")
    outg.add_argument("--report", default="", help="Write machine-readable JSON report to this path")
    outg.add_argument("--quiet", action="store_true", help="Only print summary")
    outg.add_argument("--verbose", action="store_true", help="Print per-file status")
    outg.add_argument("--max-issues", type=int, default=50, help="Max issues to print (default 50)")

    ns = ap.parse_args(list(argv) if argv is not None else sys.argv[1:])

    schema_path = Path(ns.schema).expanduser().resolve()
    if not schema_path.exists() or not schema_path.is_file():
        eprint(f"error: schema not found: {schema_path}")
        return 2

    include_globs = split_globs(ns.include)
    exclude_globs = split_globs(ns.exclude)

    root = Path(ns.in_dir).expanduser().resolve() if ns.in_dir else None
    files = [Path(p).expanduser().resolve() for p in ns.files]

    try:
        inputs = pick_inputs(root=root, files=files, include_globs=include_globs, exclude_globs=exclude_globs)
    except ValueError as e:
        eprint(f"error: {e}")
        return 2

    if not inputs:
        eprint("warning: no inputs matched")
        return 0

    try:
        schema = load_json(schema_path)
    except ValueError as e:
        eprint(f"error: {e}")
        return 2

    # Choose validator implementation
    have_jsonschema = True
    try:
        import jsonschema  # type: ignore  # noqa: F401
    except Exception:
        have_jsonschema = False

    res = Result()

    if not ns.quiet:
        impl = "jsonschema" if have_jsonschema else "minimal"
        print(f"validate_json_schema: schema={schema_path} impl={impl} inputs={len(inputs)}")

    for p in inputs:
        try:
            inst = load_json(p)

            if have_jsonschema:
                issues = validate_with_jsonschema(schema, inst)
            else:
                issues = minimal_shape_check(schema, inst)

            if issues:
                res.invalid += 1
                for jp, msg in issues:
                    res.issues.append(Issue(path=p.as_posix(), message=msg, json_path=jp))

                if ns.verbose and not ns.quiet:
                    print(f"[invalid] {p}")
            else:
                res.valid += 1
                if ns.verbose and not ns.quiet:
                    print(f"[ok] {p}")

        except ValueError as e:
            res.errors += 1
            res.issues.append(Issue(path=p.as_posix(), message=str(e), json_path=""))
            if ns.verbose and not ns.quiet:
                print(f"[error] {p}")

        except RuntimeError as e:
            # missing jsonschema
            res.errors += 1
            res.issues.append(Issue(path=p.as_posix(), message=str(e), json_path=""))
            have_jsonschema = False
            if ns.verbose and not ns.quiet:
                print(f"[error] {p}")

        except Exception as e:
            res.errors += 1
            res.issues.append(Issue(path=p.as_posix(), message=f"unexpected: {e}", json_path=""))
            if not ns.quiet:
                eprint("unexpected error:")
                traceback.print_exc()
            # continue

    total = res.valid + res.invalid + res.errors

    # Print issues (bounded)
    if not ns.quiet and res.issues:
        limit = max(0, int(ns.max_issues))
        shown = 0
        for it in res.issues:
            if shown >= limit:
                eprint(f"... ({len(res.issues) - limit} more issues suppressed)")
                break
            where = f"{it.path}"
            if it.json_path:
                where += f"::{it.json_path}"
            eprint(f"{where}: {it.message}")
            shown += 1

    if ns.report:
        try:
            emit_report_json(res, Path(ns.report).expanduser().resolve())
        except Exception as e:
            eprint(f"error writing report: {e}")
            return 3

    if not ns.quiet:
        print(
            f"validate_json_schema: total={total} valid={res.valid} invalid={res.invalid} errors={res.errors}"
        )

    if res.invalid:
        return 1
    if res.errors:
        return 2
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SystemExit:
        raise
    except Exception:
        traceback.print_exc()
        raise SystemExit(3)