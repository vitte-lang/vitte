"""Check that every diagnostic format exposes the same public metadata."""

from pathlib import Path
import json
import re
import subprocess
import sys
import tempfile


binary = Path(sys.argv[1]).resolve()
format_binary = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else binary
root = Path(__file__).resolve().parents[2]
registry = root / "src/bootstrap/diagnostic/registry.c"


def run_diagnostic(path, fmt, executable=None):
    command = [
        str(executable or format_binary),
        "check",
        str(path),
        "--quiet",
        "--no-color",
        f"--diagnostic={fmt}",
    ]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 1, (
        command,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    if fmt == "terminal":
        return result.stderr
    try:
        return json.loads(result.stderr)
    except json.JSONDecodeError as error:
        raise AssertionError((fmt, result.stderr)) from error


def first_json_diagnostic(document, code):
    matches = [item for item in document["diagnostics"] if item["code"] == code]
    assert len(matches) == 1, (code, document)
    return matches[0]


def first_sarif_result(document, code):
    results = document["runs"][0]["results"]
    matches = [item for item in results if item["ruleId"] == code]
    assert len(matches) == 1, (code, document)
    return matches[0]


def first_lsp_diagnostic(document, code):
    matches = [item for item in document if item["code"] == code]
    assert len(matches) == 1, (code, document)
    return matches[0]


def check_native_structured_case(path, source, code, phase, help_fragment):
    path.write_text(source)
    json_document = run_diagnostic(path, "json", binary)
    sarif_document = run_diagnostic(path, "sarif", binary)
    lsp_document = run_diagnostic(path, "lsp", binary)

    diagnostic = first_json_diagnostic(json_document, code)
    sarif = first_sarif_result(sarif_document, code)
    lsp = first_lsp_diagnostic(lsp_document, code)
    properties = sarif["properties"]
    lsp_data = lsp["data"]

    # Native JSON has its own compact API schema; SARIF and LSP use their
    # public renderer schemas. Assert the common metadata in each contract.
    assert diagnostic["severity"] == "error", diagnostic
    assert diagnostic["phase"] == phase.replace("-", "_"), diagnostic
    assert diagnostic["internal_code"], diagnostic
    assert diagnostic["message"], diagnostic
    assert help_fragment in diagnostic.get("help", ""), diagnostic
    assert sarif["level"] == properties["severity"] == "error", sarif
    assert properties["phase"] == lsp_data["phase"] == phase, (sarif, lsp)
    assert properties["help"] == lsp_data["help"], (sarif, lsp)
    assert help_fragment in properties["help"], properties
    assert sarif["ruleId"] == lsp["code"] == diagnostic["code"] == code

    # JSON and LSP use zero-based line/column values, while SARIF uses
    # one-based source regions. The LSP data mirror also retains raw spans.
    location = diagnostic["location"]
    region = sarif["locations"][0]["physicalLocation"]["region"]
    lsp_range = lsp["range"]
    position = lsp_data["position"]
    assert location["source"] == str(path), location
    assert region["startLine"] == location["start"]["line"] + 1
    assert region["startColumn"] == location["start"]["column"] + 1
    assert region["endLine"] == location["end"]["line"] + 1
    assert region["endColumn"] == location["end"]["column"] + 1
    assert lsp_range["start"]["line"] == location["start"]["line"]
    assert lsp_range["start"]["character"] == location["start"]["column"]
    assert lsp_range["end"]["line"] == location["end"]["line"]
    assert lsp_range["end"]["character"] == location["end"]["column"]
    assert position["source"] == location["source"]
    assert position["startLine"] == location["start"]["line"]
    assert position["startColumn"] == location["start"]["column"]
    assert position["endLine"] == location["end"]["line"]
    assert position["endColumn"] == location["end"]["column"]

    labels = [label for label in lsp_data["labels"] if label["style"] == "primary"]
    assert len(labels) == 1, lsp_data
    label = labels[0]
    assert label["source"] == location["source"]
    assert region["byteOffset"] == label["startByte"]
    assert region["byteLength"] == label["endByte"] - label["startByte"]

    if code == "E0602":
        related = lsp["relatedInformation"]
        assert any(item["message"] == "parameter declared here" for item in related), lsp


def check_explain_for_every_public_code():
    # registry.c is the authoritative source for the public code set.  Keep
    # this check at the CLI boundary so a code cannot be present in the
    # registry while being rejected by `vitte explain`.
    codes = []
    for code in re.findall(r'"([EWNH][0-9]{4})"', registry.read_text()):
        if code not in codes:
            codes.append(code)
    assert codes, "the diagnostic registry contains no public codes"

    for code in codes:
        explanations = []
        for executable in (binary, format_binary):
            structured = subprocess.run(
                [str(executable), "explain", code, "--diagnostic=json"],
                capture_output=True,
                text=True,
            )
            assert structured.returncode == 0, (
                executable,
                code,
                structured.stdout,
                structured.stderr,
            )
            assert not structured.stderr, (executable, code, structured.stderr)
            document = json.loads(structured.stdout)
            entry = document["diagnostic"]
            required_text = (
                "internal", "title", "explanation", "severity", "phase",
                "category", "guidance",
            )
            assert entry["code"] == code, (executable, code, document)
            for field in required_text:
                assert isinstance(entry.get(field), str) and entry[field].strip(), (
                    executable,
                    code,
                    field,
                    entry,
                )
            assert len(entry["guidance"].strip()) >= 12, (
                executable,
                code,
                entry["guidance"],
            )
            assert entry["severity"] in {"error", "fatal", "warning", "note", "help"}, (
                executable,
                code,
                entry,
            )
            assert isinstance(entry.get("deprecated"), bool), (executable, code, entry)
            explanations.append(entry)

        # The bootstrap C frontend and the native Vitte frontend share one
        # diagnostic contract, including the prose users get from `explain`.
        fields = (
            "code", "internal", "title", "explanation", "severity", "phase",
            "category", "guidance", "deprecated",
        )
        assert tuple(explanations[0][field] for field in fields) == tuple(
            explanations[1][field] for field in fields
        ), (code, explanations)

        text = subprocess.run(
            [str(binary), "explain", code],
            capture_output=True,
            text=True,
        )
        assert text.returncode == 0, (code, text.stdout, text.stderr)
        entry = explanations[0]
        for field in ("code", "title", "explanation", "guidance"):
            assert entry[field] in text.stdout, (code, field, text.stdout)


def check_format_parity():
    source = (
        "proc add(a: int) -> int { return a; }\n"
        "proc main() -> int { return add(\"42\"); }\n"
    )

    with tempfile.TemporaryDirectory(prefix="vitte-diagnostic-parity-") as directory:
        path = Path(directory) / "case.vit"
        path.write_text(source)

        terminal = run_diagnostic(path, "terminal")
        native_terminal = run_diagnostic(path, "terminal", binary)
        json_document = run_diagnostic(path, "json")
        sarif_document = run_diagnostic(path, "sarif")
        lsp_document = run_diagnostic(path, "lsp")

        code = "E0602"
        diagnostic = first_json_diagnostic(json_document, code)
        sarif = first_sarif_result(sarif_document, code)
        lsp = first_lsp_diagnostic(lsp_document, code)
        lsp_data = lsp["data"]
        context = diagnostic["context"]
        position = diagnostic["position"]
        properties = sarif["properties"]
        lsp_position = lsp_data["position"]

        # Public identity and diagnostic semantics must be stable across all
        # renderers.  LSP severity is numeric by protocol, so compare its
        # data mirror with the textual/structured severity name.
        assert diagnostic["code"] == sarif["ruleId"] == lsp["code"] == code
        assert diagnostic["severity"] == properties["severity"] == lsp_data["severity"] == "error"
        assert diagnostic["phase"] == properties["phase"] == lsp_data["phase"] == "type-check"
        assert diagnostic["category"] == properties["category"] == lsp_data["category"] == "call"
        assert context["expected_type"] == properties["expectedType"] == lsp_data["expectedType"] == "i64"
        assert context["actual_type"] == properties["actualType"] == lsp_data["actualType"] == "string"
        help_text = diagnostic["annotations"][0]["message"]
        assert properties["help"] == lsp_data["help"] == help_text
        assert diagnostic["causes"] == properties["causes"] == lsp_data["causes"] == []

        # JSON/SARIF retain source byte spans.  LSP retains the same raw span
        # in data.labels while exposing the converted UTF-16 range required by
        # the protocol.
        sarif_region = sarif["locations"][0]["physicalLocation"]["region"]
        primary_label = next(
            label for label in lsp_data["labels"] if label["style"] == "primary"
        )
        assert position["source"] == sarif["locations"][0]["physicalLocation"]["artifactLocation"]["uri"]
        assert position["source"] == primary_label["source"]
        assert lsp_position["source"] == position["source"]
        assert lsp_position["hasSpan"] == position["has_span"]
        assert lsp_position["startByte"] == position["start_byte"]
        assert lsp_position["endByte"] == position["end_byte"]
        assert lsp_position["startLine"] == position["start_line"]
        assert lsp_position["startColumn"] == position["start_column"]
        assert lsp_position["endLine"] == position["end_line"]
        assert lsp_position["endColumn"] == position["end_column"]
        assert position["start_byte"] == sarif_region["byteOffset"] == primary_label["startByte"]
        assert position["end_byte"] - position["start_byte"] == sarif_region["byteLength"]
        assert position["end_byte"] == primary_label["endByte"]
        assert sarif_region["startLine"] == position["start_line"] + 1
        assert sarif_region["startColumn"] == position["start_column"] + 1
        assert sarif_region["endLine"] == position["end_line"] + 1
        assert sarif_region["endColumn"] == position["end_column"] + 1
        assert lsp["range"]["start"]["line"] == position["start_line"]
        assert lsp["range"]["start"]["character"] == position["start_column"]
        assert lsp["range"]["end"]["line"] == position["end_line"]
        assert lsp["range"]["end"]["character"] == position["end_column"]

        # The terminal renderer must carry the same user-facing context.
        assert code in terminal
        assert "phase: type-check" in terminal
        assert "expected `i64`, found `string`" in terminal
        assert help_text in terminal
        assert str(path) in terminal

        # The native CLI must use the terminal renderer too, including the
        # actionable help and source excerpt, instead of leaking API JSON.
        assert code in native_terminal
        assert "phase: type-check" in native_terminal
        assert "expected `i64`, found `string`" in native_terminal
        assert help_text in native_terminal
        assert str(path) in native_terminal
        assert 'return add("42");' in native_terminal

        # Exercise all structured formats on the native frontend across lexer,
        # parser, and semantic diagnostics, including the registry help fallback
        # used by native JSON when a frontend diagnostic has no inline help.
        check_native_structured_case(
            path,
            'proc main() -> int { let value = "bad\\q"; }\n',
            "E0103",
            "lexer",
            "supported escape sequence",
        )
        check_native_structured_case(
            path,
            "proc main() -> int { return (1 + 2; }\n",
            "E0201",
            "parser",
            "`)`",
        )
        check_native_structured_case(
            path,
            source,
            "E0602",
            "type-check",
            "Argument 1 does not match parameter `a`",
        )

        # Related declaration locations are part of the parity contract too.
        related_json = diagnostic["related_locations"][0]["span"]
        related_sarif = sarif["relatedLocations"][0]["physicalLocation"]
        related_lsp = lsp["relatedInformation"][0]
        related_region = related_sarif["region"]
        assert related_json["source"] == related_sarif["artifactLocation"]["uri"]
        assert related_json["start_byte"] == related_region["byteOffset"]
        assert related_json["end_byte"] - related_json["start_byte"] == related_region["byteLength"]
        assert related_lsp["location"]["uri"] == related_json["source"]
        assert related_lsp["location"]["range"]["start"]["line"] == related_json["start_line"]
        assert related_lsp["location"]["range"]["start"]["character"] == related_json["start_column"]


check_explain_for_every_public_code()
check_format_parity()
print("Diagnostic parity: explain, terminal, JSON, SARIF and LSP passed.")
