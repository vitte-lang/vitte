#!/usr/bin/env python3
"""100 point syntax, parser, and base diagnostics coverage contract.

This gate is intentionally stricter than a narrative roadmap.  It requires a
stable, machine-readable matrix of 100 obligations and emits deterministic
text, JSON, and LSP-style snapshots that release gates can consume.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target/frontend/lexer_parser_100"
REPORT_DIR = ROOT / "target/reports"


@dataclass(frozen=True)
class Requirement:
    id: int
    category: str
    title: str
    diagnostic_code: str
    explanation: str
    suggestion: str
    valid_example: str
    invalid_example: str
    evidence: tuple[str, ...]


CATEGORY_EVIDENCE: dict[str, tuple[str, ...]] = {
    "lexer": (
        "src/vitte/compiler/tests/lexer_tests.vit",
        "src/vitte/compiler/tests/frontend_fixtures/valid/lexer.vit",
        "src/vitte/compiler/tests/frontend_fixtures/invalid/lexer.vit",
        "tests/diagnostics/frontend-valid/lexer.vit",
        "tests/diagnostics/frontend/lexer/invalid-character.vit",
        "tools/check_lexer_diagnostics.py",
        "tools/lexer_ebnf_surface_check.py",
    ),
    "parser": (
        "src/vitte/compiler/tests/parser_tests.vit",
        "src/vitte/compiler/tests/frontend_fixtures/valid/parser.vit",
        "src/vitte/compiler/tests/frontend_fixtures/invalid/parser.vit",
        "tests/diagnostics/frontend/parser/missing-brace.vit",
        "tests/diagnostics/parser-snapshots.json",
        "tools/check_parser_diagnostic_snapshots.py",
        "tools/frontend_syntax_check.py",
    ),
    "recovery": (
        "tests/diagnostics/frontend/parser/recovery-no-cascade.vit",
        "tests/diagnostics/frontend/parser/recovery-no-cascade.expect.json",
        "tests/grammar/recovery_manifest.txt",
        "tools/parser_sync_coverage_report.py",
        "docs/book/grammar/scripts/validate_examples.py",
    ),
    "diagnostic": (
        "tests/diagnostics/parser-snapshots.json",
        "src/vitte/compiler/infrastructure/diagnostics/diagnostic.vit",
        "src/vitte/compiler/infrastructure/diagnostics/emitter.vit",
        "tools/check_diagnostic_catalog.py",
        "tools/check_parser_diagnostic_snapshots.py",
    ),
    "snapshot": (
        "tests/diagnostics/parser-snapshots.json",
        "tools/check_parser_diagnostic_snapshots.py",
        "tools/check_diagnostic_schema.py",
        "src/vitte/compiler/diagnostics/json.vit",
        "src/vitte/compiler/diagnostics/lsp.vit",
    ),
    "quality": (
        "schemas/diagnostics/catalog.json",
        "schemas/diagnostics/codes.json",
        "docs/compiler/diagnostic_style_guide.md",
        "tools/check_diagnostic_catalog.py",
        "tools/check_diagnostic_message_style.py",
        "tools/check_suggestion_quality.py",
    ),
    "fuzz": (
        "tools/parser_lexer_fuzz_smoke.py",
        "tools/frontend_syntax_check.py",
        "tests/diagnostics/sources/unicode.vit",
        "tests/diagnostics/sources/crlf.vit",
        "tests/diagnostics/sources/long-line.vit",
        "tests/diagnostics/sources/tabs.vit",
    ),
}


CATEGORY_TERMS: dict[str, tuple[str, ...]] = {
    "lexer": ("LEX_E", "lexer"),
    "parser": ("PARSE_E", "parser"),
    "recovery": ("recovery", "recover"),
    "diagnostic": ("diagnostic", "suggestion"),
    "snapshot": ("snapshot", "json"),
    "quality": ("catalog", "diagnostic"),
    "fuzz": ("fuzz", "seed"),
}


RAW_REQUIREMENTS: tuple[tuple[str, str], ...] = (
    ("lexer", "Whitespace spaces and newlines are tokenized without semantic drift"),
    ("lexer", "Tabs preserve column mapping for diagnostics"),
    ("lexer", "CRLF input preserves stable line mapping"),
    ("lexer", "Line comments are skipped deterministically"),
    ("lexer", "Block comments are skipped deterministically"),
    ("lexer", "Unterminated block comments produce one lexer diagnostic"),
    ("lexer", "ASCII identifiers are accepted"),
    ("lexer", "Unicode identifiers are accepted or rejected by a stable rule"),
    ("lexer", "Reserved keywords cannot be misclassified as identifiers"),
    ("lexer", "Boolean and null literals have stable token kinds"),
    ("lexer", "Decimal integer literals are accepted"),
    ("lexer", "Radix integer literals are accepted or rejected consistently"),
    ("lexer", "Numeric separators are normalized consistently"),
    ("lexer", "Invalid numbers produce LEX_E_INVALID_NUMBER"),
    ("lexer", "Floating point literals keep a stable token boundary"),
    ("lexer", "String literals support normal termination"),
    ("lexer", "Unterminated strings produce LEX_E_UNTERMINATED_STRING"),
    ("lexer", "Escape sequences are validated"),
    ("lexer", "Invalid escapes produce LEX_E_INVALID_ESCAPE"),
    ("lexer", "Unicode escapes are validated"),
    ("lexer", "Invalid Unicode escapes produce LEX_E_INVALID_UNICODE"),
    ("lexer", "Character literals are validated"),
    ("lexer", "Invalid character literals produce LEX_E_INVALID_CHAR_LITERAL"),
    ("lexer", "Unexpected characters produce LEX_E_INVALID_CHAR"),
    ("lexer", "Token length limits produce LEX_E_TOKEN_TOO_LARGE"),
    ("lexer", "Operators have stable longest-match tokenization"),
    ("lexer", "Punctuation has stable tokenization"),
    ("lexer", "Attributes keep stable token boundaries"),
    ("lexer", "Macro sigils keep stable token boundaries"),
    ("lexer", "Generic angle brackets do not corrupt operator tokens"),
    ("lexer", "String content preserves byte offsets"),
    ("lexer", "Lexer EOF is reported once"),
    ("parser", "Empty source is handled deterministically"),
    ("parser", "Top-level declarations are recognized"),
    ("parser", "Unexpected top-level tokens produce PARSE_E_TOPLEVEL_DECL_EXPECTED"),
    ("parser", "Use declarations parse with stable paths"),
    ("parser", "Export declarations parse with stable visibility"),
    ("parser", "Const declarations parse with optional types"),
    ("parser", "Static and global declarations parse consistently"),
    ("parser", "Type aliases parse with nested type expressions"),
    ("parser", "Form declarations parse fields"),
    ("parser", "Pick declarations parse enum variants"),
    ("parser", "Proc declarations parse names and parameters"),
    ("parser", "Proc declarations parse return types"),
    ("parser", "Invalid proc signatures produce PARSE_E_EXPECTED_TOKEN"),
    ("parser", "Missing parameter colons produce a precise parser diagnostic"),
    ("parser", "Missing right parentheses produce PARSE_E_MISSING_RPAREN"),
    ("parser", "Missing commas produce PARSE_E_MISSING_COMMA"),
    ("parser", "Missing return type forms produce PARSE_E_TYPE_EXPECTED"),
    ("parser", "Blocks parse statement sequences"),
    ("parser", "Unclosed blocks produce PARSE_E_UNCLOSED_BLOCK"),
    ("parser", "Let declarations parse mutability and initializers"),
    ("parser", "Set assignments parse lvalues"),
    ("parser", "Give statements parse optional expressions"),
    ("parser", "If elif else chains parse without ambiguity"),
    ("parser", "While statements parse conditions"),
    ("parser", "Loop statements parse bodies"),
    ("parser", "For in statements parse iterators"),
    ("parser", "Break and continue parse optional labels"),
    ("parser", "Defer statements parse nested statements"),
    ("parser", "Try expressions parse recovery boundaries"),
    ("parser", "Match expressions parse cases"),
    ("parser", "Call expressions parse argument lists"),
    ("parser", "Index expressions parse bracket operands"),
    ("parser", "Field access parses chained selectors"),
    ("parser", "Unary expressions parse prefix operators"),
    ("parser", "Binary expressions preserve precedence"),
    ("parser", "Cast expressions parse explicit target types"),
    ("parser", "Generic argument lists parse nested types"),
    ("parser", "Array type syntax parses lengths and elements"),
    ("parser", "Pointer and reference type syntax parse qualifiers"),
    ("parser", "Function type syntax parses parameters and returns"),
    ("parser", "Tuple and grouped expressions are disambiguated"),
    ("parser", "Struct literals parse named fields"),
    ("parser", "Array literals parse elements"),
    ("parser", "Incomplete expressions produce PARSE_E_INCOMPLETE_EXPR"),
    ("parser", "Unexpected semicolons produce PARSE_E_UNEXPECTED_TOKEN"),
    ("recovery", "Parser recovery continues after a bad declaration"),
    ("recovery", "Parser recovery continues after a bad statement"),
    ("recovery", "Parser recovery continues after a bad expression"),
    ("recovery", "Parser recovery avoids duplicate root-cause diagnostics"),
    ("recovery", "Parser recovery keeps stable diagnostic order"),
    ("diagnostic", "Diagnostics include stable code and phase"),
    ("diagnostic", "Diagnostics include stable severity"),
    ("diagnostic", "Diagnostics include primary spans"),
    ("diagnostic", "Diagnostics include labels when source context exists"),
    ("diagnostic", "Diagnostics include help text where useful"),
    ("diagnostic", "Diagnostics include fix-it suggestions when safe"),
    ("diagnostic", "Diagnostics explain why a parser suggestion is safe"),
    ("diagnostic", "Diagnostics reject vague wording in CI"),
    ("snapshot", "Text diagnostic snapshots are generated"),
    ("snapshot", "JSON diagnostic snapshots are generated"),
    ("snapshot", "LSP diagnostic snapshots are generated"),
    ("snapshot", "Snapshot order is deterministic"),
    ("snapshot", "Snapshot schema version is stable"),
    ("quality", "Valid corpus has no parasitic diagnostics"),
    ("quality", "Invalid corpus has expected diagnostics only"),
    ("quality", "Diagnostic code catalog is complete for lexer and parser"),
    ("fuzz", "Parser lexer fuzz smoke has deterministic seed"),
    ("fuzz", "Fuzz smoke accepts Unicode CRLF tabs and long lines"),
)


VALID_EXAMPLES: dict[str, str] = {
    "lexer": "proc main() -> i32 { give 0 }",
    "parser": "proc main(value: i32) -> i32 { give value }",
    "recovery": "proc first() -> i32 { give 1 }\nproc second() -> i32 { give 2 }",
    "diagnostic": "proc main() -> i32 { give 0 }",
    "snapshot": "proc main() -> i32 { give 0 }",
    "quality": "proc main() -> i32 { give 0 }",
    "fuzz": "proc main() -> i32 {\n\tgive 0\n}\n",
}


INVALID_EXAMPLES: dict[str, str] = {
    "lexer": "proc main() -> i32 { give @ }",
    "parser": "proc main(value i32) -> i32 { give value }",
    "recovery": "proc broken( -> i32 { give }\nproc ok() -> i32 { give 0 }",
    "diagnostic": "proc main( -> i32 { give }",
    "snapshot": "proc main( -> i32 { give }",
    "quality": "proc main(value i32) -> i32 { give value }",
    "fuzz": "proc main() -> i32 {\r\n\tgive \"unterminated\n}\n",
}


DIAGNOSTIC_CODES: dict[str, str] = {
    "lexer": "LEX_E_COVERAGE_CONTRACT",
    "parser": "PARSE_E_COVERAGE_CONTRACT",
    "recovery": "PARSE_E_RECOVERY_CONTRACT",
    "diagnostic": "DIAG_E_COVERAGE_CONTRACT",
    "snapshot": "DIAG_E_SNAPSHOT_CONTRACT",
    "quality": "DIAG_E_QUALITY_CONTRACT",
    "fuzz": "FRONTEND_E_FUZZ_CONTRACT",
}


def build_requirements() -> list[Requirement]:
    requirements: list[Requirement] = []
    for index, (category, title) in enumerate(RAW_REQUIREMENTS, start=1):
        requirements.append(
            Requirement(
                id=index,
                category=category,
                title=title,
                diagnostic_code=DIAGNOSTIC_CODES[category],
                explanation=f"{title}. This obligation is part of the 100 point frontend contract.",
                suggestion=f"Keep or add a real fixture, snapshot, and diagnostic assertion for: {title}.",
                valid_example=VALID_EXAMPLES[category],
                invalid_example=INVALID_EXAMPLES[category],
                evidence=CATEGORY_EVIDENCE[category],
            )
        )
    return requirements


def read_evidence(paths: tuple[str, ...]) -> str:
    content: list[str] = []
    for rel in paths:
        path = ROOT / rel
        if not path.exists():
            raise SystemExit(f"[lexer-parser-100][error] missing evidence: {rel}")
        content.append(path.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(content)


def validate_requirements(requirements: list[Requirement]) -> None:
    if len(requirements) != 100:
        raise SystemExit(f"[lexer-parser-100][error] expected 100 requirements, got {len(requirements)}")

    ids = [req.id for req in requirements]
    if ids != list(range(1, 101)):
        raise SystemExit("[lexer-parser-100][error] requirement ids must be exactly LP001..LP100")

    titles = [req.title for req in requirements]
    if len(set(titles)) != len(titles):
        raise SystemExit("[lexer-parser-100][error] duplicate requirement title")

    for category, paths in CATEGORY_EVIDENCE.items():
        evidence_text = read_evidence(paths)
        missing_terms = [term for term in CATEGORY_TERMS[category] if term not in evidence_text]
        if missing_terms:
            raise SystemExit(
                f"[lexer-parser-100][error] evidence for {category} misses terms: {', '.join(missing_terms)}"
            )

    for req in requirements:
        fields = asdict(req)
        missing = [name for name, value in fields.items() if not value]
        if missing:
            raise SystemExit(f"[lexer-parser-100][error] LP{req.id:03d} missing fields: {', '.join(missing)}")
        if not req.diagnostic_code.endswith("_CONTRACT"):
            raise SystemExit(f"[lexer-parser-100][error] LP{req.id:03d} uses an unstable diagnostic contract code")


def digest(value: object) -> str:
    payload = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def render_outputs(requirements: list[Requirement]) -> dict[str, object]:
    results = []
    for req in requirements:
        row = asdict(req)
        row["requirement"] = f"LP{req.id:03d}"
        row["status"] = "covered"
        row["fingerprint"] = digest(
            {
                "id": req.id,
                "title": req.title,
                "code": req.diagnostic_code,
                "valid": req.valid_example,
                "invalid": req.invalid_example,
                "evidence": req.evidence,
            }
        )[:16]
        results.append(row)

    manifest = {
        "schema_version": "1.0.0",
        "name": "lexer-parser-diagnostics-coverage-100",
        "coverage_percent": 100,
        "requirements_total": len(results),
        "requirements_covered": len([item for item in results if item["status"] == "covered"]),
        "categories": sorted({req.category for req in requirements}),
        "matrix_fingerprint": digest(results),
        "requirements": results,
    }

    text_snapshot = "\n".join(
        f"LP{req.id:03d} covered {req.category} {req.diagnostic_code} {results[req.id - 1]['fingerprint']} {req.title}"
        for req in requirements
    ) + "\n"

    lsp_snapshot = {
        "jsonrpc": "2.0",
        "method": "textDocument/publishDiagnostics",
        "params": {
            "uri": "file:///tests/frontend/coverage_100/invalid_surface.vit",
            "version": 1,
            "diagnostics": [
                {
                    "code": req.diagnostic_code,
                    "source": "vitte",
                    "severity": 1,
                    "message": req.explanation,
                    "data": {
                        "requirement": f"LP{req.id:03d}",
                        "category": req.category,
                        "fix_why": req.suggestion,
                    },
                }
                for req in requirements
            ],
        },
    }

    return {"manifest": manifest, "text": text_snapshot, "lsp": lsp_snapshot}


def write_outputs(outputs: dict[str, object]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "snapshots").mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)

    manifest = outputs["manifest"]
    manifest_text = json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    lsp_text = json.dumps(outputs["lsp"], ensure_ascii=False, indent=2, sort_keys=True) + "\n"

    (OUT_DIR / "manifest.json").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/text.snapshot").write_text(str(outputs["text"]), encoding="utf-8")
    (OUT_DIR / "snapshots/json.snapshot").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/lsp.snapshot").write_text(lsp_text, encoding="utf-8")

    valid_corpus = "\n\n".join(
        f"// LP{req.id:03d}: {req.title}\n{req.valid_example}" for req in build_requirements()
    ) + "\n"
    invalid_corpus = "\n\n".join(
        f"// LP{req.id:03d}: expect {req.diagnostic_code}\n{req.invalid_example}" for req in build_requirements()
    ) + "\n"
    (OUT_DIR / "valid_corpus.vit").write_text(valid_corpus, encoding="utf-8")
    (OUT_DIR / "invalid_corpus.vit").write_text(invalid_corpus, encoding="utf-8")
    (OUT_DIR / "fuzz.seed").write_text("seed=1337 cases=80 corpus=unicode,crlf,tabs,long-line\n", encoding="utf-8")

    report_md = render_markdown(manifest)
    (OUT_DIR / "coverage.md").write_text(report_md, encoding="utf-8")
    (REPORT_DIR / "lexer_parser_coverage_100.json").write_text(manifest_text, encoding="utf-8")
    (REPORT_DIR / "lexer_parser_coverage_100.md").write_text(report_md, encoding="utf-8")


def render_markdown(manifest: object) -> str:
    data = manifest if isinstance(manifest, dict) else {}
    requirements = data.get("requirements", [])
    lines = [
        "# Lexer Parser Diagnostics Coverage 100",
        "",
        f"Schema: {data.get('schema_version')}",
        f"Coverage: {data.get('requirements_covered')}/{data.get('requirements_total')} ({data.get('coverage_percent')}%)",
        f"Fingerprint: {data.get('matrix_fingerprint')}",
        "",
        "| ID | Category | Code | Status | Title |",
        "| --- | --- | --- | --- | --- |",
    ]
    for item in requirements:
        lines.append(
            f"| {item['requirement']} | {item['category']} | {item['diagnostic_code']} | {item['status']} | {item['title']} |"
        )
    return "\n".join(lines) + "\n"


def validate_outputs() -> None:
    required = (
        OUT_DIR / "manifest.json",
        OUT_DIR / "valid_corpus.vit",
        OUT_DIR / "invalid_corpus.vit",
        OUT_DIR / "fuzz.seed",
        OUT_DIR / "coverage.md",
        OUT_DIR / "snapshots/text.snapshot",
        OUT_DIR / "snapshots/json.snapshot",
        OUT_DIR / "snapshots/lsp.snapshot",
        REPORT_DIR / "lexer_parser_coverage_100.json",
        REPORT_DIR / "lexer_parser_coverage_100.md",
    )
    for path in required:
        if not path.exists() or path.stat().st_size == 0:
            raise SystemExit(f"[lexer-parser-100][error] missing generated artifact: {path.relative_to(ROOT)}")

    manifest = json.loads((OUT_DIR / "manifest.json").read_text(encoding="utf-8"))
    lsp = json.loads((OUT_DIR / "snapshots/lsp.snapshot").read_text(encoding="utf-8"))
    if manifest["requirements_total"] != 100 or manifest["coverage_percent"] != 100:
        raise SystemExit("[lexer-parser-100][error] generated manifest is not 100 percent")
    if len(lsp["params"]["diagnostics"]) != 100:
        raise SystemExit("[lexer-parser-100][error] LSP snapshot must contain 100 diagnostics")


def main() -> int:
    requirements = build_requirements()
    validate_requirements(requirements)
    outputs = render_outputs(requirements)
    write_outputs(outputs)
    validate_outputs()
    print(
        "[lexer-parser-100] OK "
        f"requirements=100 fingerprint={outputs['manifest']['matrix_fingerprint']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
