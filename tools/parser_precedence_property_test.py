#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import random
import subprocess
import tempfile
from pathlib import Path


def load_precedence(repo: Path) -> dict[str, tuple[int, str]]:
    payload = json.loads((repo / "docs/book/grammar/precedence_table.json").read_text(encoding="utf-8"))
    rows = payload.get("rows", [])
    out: dict[str, tuple[int, str]] = {}
    for row in rows:
        if not isinstance(row, dict):
            continue
        if not bool(row.get("core", False)):
            continue
        op = str(row.get("op", "")).split("::")[-1]
        prec = int(row.get("precedence", 0))
        assoc = str(row.get("assoc", "left"))
        if op:
            prev = out.get(op)
            if prev is None:
                out[op] = (prec, assoc)
            else:
                prev_prec, prev_assoc = prev
                merged_prec = max(prev_prec, prec)
                merged_assoc = "right" if (prev_assoc == "right" or assoc == "right") else "left"
                out[op] = (merged_prec, merged_assoc)
    return out


def parse_ast(vitte_bin: Path, src: str) -> dict:
    with tempfile.NamedTemporaryFile("w", suffix=".vit", delete=False, encoding="utf-8") as tmp:
        tmp.write(src)
        tmp_path = Path(tmp.name)
    try:
        cmd = [str(vitte_bin), "parse", "--parse-silent", "--dump-ast-json", str(tmp_path)]
        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        out = (proc.stdout or "") + "\n" + (proc.stderr or "")
        start = out.find("{")
        if start < 0:
            raise RuntimeError(f"no AST json emitted (rc={proc.returncode})")
        dec = json.JSONDecoder()
        payload, _ = dec.raw_decode(out[start:])
        if not isinstance(payload, dict):
            raise RuntimeError("invalid AST json payload")
        return payload
    finally:
        tmp_path.unlink(missing_ok=True)


def has_detailed_ast_surface(vitte_bin: Path, repo: Path) -> bool:
    src = repo / "toolchain/stage2/src/main.vit"
    cmd = [str(vitte_bin), "parse", "--dump-ast-json", str(src)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    out = (proc.stdout or "") + "\n" + (proc.stderr or "")
    if proc.returncode != 0:
        return False
    start = out.find("{")
    if start < 0:
        return False
    try:
        payload, _ = json.JSONDecoder().raw_decode(out[start:])
    except json.JSONDecodeError:
        return False
    if not isinstance(payload, dict):
        return False
    ast = payload.get("ast")
    if isinstance(ast, dict) and ast.get("kind") == "bootstrap-structural":
        return False
    return True


def find_first(node: dict, kind: str) -> dict | None:
    if node.get("kind") == kind:
        return node
    for child in node.get("children", []):
        if isinstance(child, dict):
            got = find_first(child, kind)
            if got is not None:
                return got
    return None


def is_expr_kind(kind: str) -> bool:
    return kind.endswith("Expr")


def let_initializer_expr(module_json: dict) -> dict:
    let_node = find_first(module_json, "LetStmt")
    if let_node is None:
        raise RuntimeError("LetStmt not found in parsed AST")
    expr_children = [c for c in let_node.get("children", []) if isinstance(c, dict) and is_expr_kind(str(c.get("kind", "")))]
    if not expr_children:
        raise RuntimeError("LetStmt initializer expression not found")
    return expr_children[-1]


def check_precedence_tree(node: dict, table: dict[str, tuple[int, str]], parent: tuple[str, int, str] | None = None, side: str = "") -> list[str]:
    errors: list[str] = []
    kind = str(node.get("kind", ""))
    if kind == "BinaryExpr":
        op = str(node.get("data", {}).get("op", ""))
        if op not in table:
            errors.append(f"unknown binary op in AST: {op}")
            return errors
        prec, assoc = table[op]
        if parent is not None:
            pop, pprec, passoc = parent
            if prec < pprec:
                errors.append(f"child op {op} has lower precedence than parent {pop}")
            if prec == pprec:
                if side == "left" and passoc == "right":
                    errors.append(f"left child keeps same precedence under right-assoc parent {pop}")
                if side == "right" and passoc == "left":
                    errors.append(f"right child keeps same precedence under left-assoc parent {pop}")
        children = [c for c in node.get("children", []) if isinstance(c, dict) and is_expr_kind(str(c.get("kind", "")))]
        if len(children) >= 1:
            errors.extend(check_precedence_tree(children[0], table, (op, prec, assoc), "left"))
        if len(children) >= 2:
            errors.extend(check_precedence_tree(children[1], table, (op, prec, assoc), "right"))
        return errors

    for child in node.get("children", []):
        if isinstance(child, dict):
            errors.extend(check_precedence_tree(child, table, parent, side))
    return errors


def rand_ident(rng: random.Random) -> str:
    return rng.choice(["a", "b", "c", "d", "e", "x", "y", "z"])


def rand_term(rng: random.Random) -> str:
    ident_base = rng.random() < 0.7
    base = rand_ident(rng) if ident_base else str(rng.randint(0, 9))
    if ident_base and rng.random() < 0.35:
        base = f"{base}.{rng.choice(['len', 'v', 'item'])}"
    if ident_base and rng.random() < 0.35:
        base = f"{base}[0]"
    if rng.random() < 0.30:
        base = f"{base} as int"
    if rng.random() < 0.35:
        base = f"-{base}"
    return base


def random_expr(rng: random.Random, ops: list[str]) -> str:
    n = rng.randint(3, 8)
    parts = [rand_term(rng)]
    for _ in range(n - 1):
        parts.append(rng.choice(ops))
        parts.append(rand_term(rng))
    return " ".join(parts)


def deterministic_cases() -> list[tuple[str, str, str]]:
    return [
        ("-a * b", "BinaryExpr", "Mul"),
        ("a.b + c", "BinaryExpr", "Add"),
        ("a[0] + b", "BinaryExpr", "Add"),
        ("a as int + b", "BinaryExpr", "Add"),
    ]


def main() -> int:
    ap = argparse.ArgumentParser(description="property-based precedence checks over parser AST")
    ap.add_argument("--cases", type=int, default=150)
    ap.add_argument("--seed", type=int, default=1337)
    ap.add_argument("--vitte-bin", default=os.environ.get("VITTE_BIN", "bin/vitte"))
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    vitte_bin = (repo / args.vitte_bin) if not Path(args.vitte_bin).is_absolute() else Path(args.vitte_bin)
    if not vitte_bin.exists():
        print(f"[parser-precedence-property] missing binary: {vitte_bin}")
        return 1
    if not has_detailed_ast_surface(vitte_bin, repo):
        print("[parser-precedence-property] limited: detailed AST surface is not available")
        return 0

    table = load_precedence(repo)
    ops = ["=", "or", "and", "==", "!=", "<", "<=", ">", ">=", "+", "-", "*", "/", "%"]
    if not table:
        print("[parser-precedence-property] empty precedence table")
        return 1

    rng = random.Random(args.seed)
    failures: list[str] = []

    for expr, want_kind, want_op in deterministic_cases():
        src = f"entry main at core/app {{ let v = {expr} give 0 }}\n"
        try:
            ast_json = parse_ast(vitte_bin, src)
            init = let_initializer_expr(ast_json)
            if str(init.get("kind")) != want_kind or str(init.get("data", {}).get("op", "")) != want_op:
                failures.append(f"deterministic case mismatch for '{expr}'")
        except Exception as exc:  # noqa: BLE001
            failures.append(f"deterministic case failed for '{expr}': {exc}")

    for _ in range(args.cases):
        expr = random_expr(rng, ops)
        src = f"entry main at core/app {{ let v = {expr} give 0 }}\n"
        try:
            ast_json = parse_ast(vitte_bin, src)
            init = let_initializer_expr(ast_json)
            errs = check_precedence_tree(init, table)
            if errs:
                failures.append(f"{expr} :: {'; '.join(errs)}")
        except Exception as exc:  # noqa: BLE001
            failures.append(f"{expr} :: {exc}")
        if len(failures) >= 20:
            break

    if failures:
        print("[parser-precedence-property] FAILED")
        for f in failures:
            print(f"- {f}")
        return 1

    print(f"[parser-precedence-property] OK cases={args.cases} seed={args.seed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
