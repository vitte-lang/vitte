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

    guidance_by_code = {}
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
            assert entry["guidance"].strip() not in {
                entry["title"].strip(), entry["explanation"].strip(),
            }, (executable, code, entry)
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

        canonical_guidance = explanations[0]["guidance"].strip()
        prior_code = guidance_by_code.get(canonical_guidance)
        assert prior_code is None, (
            "registered diagnostics should have code-specific remediation guidance",
            prior_code,
            code,
            canonical_guidance,
        )
        guidance_by_code[canonical_guidance] = code

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
            "2 hex digits",
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


def check_frontend_user_information_parity():
    """Compare user-facing information for a construct accepted by both frontends.

    The JSON renderers have different envelopes, so compare their common
    contract and verify related-location/help content at the terminal boundary.
    """
    source = (
        "proc add(a: int) -> int { return a; }\n"
        'proc main() -> int { return add("42"); }\n'
    )
    shared_help = "Argument 1 does not match parameter `a` of procedure `add`"

    with tempfile.TemporaryDirectory(prefix="vitte-frontend-diagnostic-parity-") as directory:
        path = Path(directory) / "parity_case.vit"
        path.write_text(source, encoding="utf-8")
        outputs = []
        for executable in (format_binary, binary):
            structured = run_diagnostic(path, "json", executable)
            diagnostic = first_json_diagnostic(structured, "E0602")
            terminal = run_diagnostic(path, "terminal", executable)
            outputs.append((diagnostic, terminal))

        bootstrap, native = outputs
        bootstrap_diagnostic, bootstrap_terminal = bootstrap
        native_diagnostic, native_terminal = native

        def phase(diagnostic):
            return diagnostic["phase"].replace("_", "-")

        assert bootstrap_diagnostic["code"] == native_diagnostic["code"] == "E0602"
        assert bootstrap_diagnostic["internal_code"] == native_diagnostic["internal_code"]
        assert bootstrap_diagnostic["severity"] == native_diagnostic["severity"] == "error"
        assert phase(bootstrap_diagnostic) == phase(native_diagnostic) == "type-check"
        assert bootstrap_diagnostic["message"] == native_diagnostic["message"] == "invalid argument type"
        assert bootstrap_diagnostic["details"] == native_diagnostic["details"] == "expected i64, found string"

        bootstrap_position = bootstrap_diagnostic["position"]
        native_location = native_diagnostic["location"]
        assert (
            bootstrap_position["start_line"], bootstrap_position["start_column"],
            bootstrap_position["end_line"], bootstrap_position["end_column"],
        ) == (
            native_location["start"]["line"], native_location["start"]["column"],
            native_location["end"]["line"], native_location["end"]["column"],
        )

        # Both help policies identify the call, parameter, and procedure. The
        # expected and actual types are present in structured details and the
        # annotated source in both frontends, even when the prose differs.
        bootstrap_help = " ".join(
            annotation["message"]
            for annotation in bootstrap_diagnostic["annotations"]
            if annotation["kind"] == "help"
        )
        native_help = native_diagnostic["help"]
        for help_text in (bootstrap_help, native_help):
            assert shared_help in help_text, help_text
        for terminal in (bootstrap_terminal, native_terminal):
            assert "phase: type-check" in terminal
            assert "category: call" in terminal
            assert "expected i64, found string" in terminal
            assert shared_help in terminal
            # The argument is line 2, column 33; its parameter declaration is
            # line 1, column 10. Both renderers should retain both locations.
            locations = set(re.findall(r"parity_case\.vit:(\d+):(\d+)", terminal))
            assert {("2", "33"), ("1", "10")} <= locations, terminal
            assert "parameter" in terminal and "declared here" in terminal

        related = bootstrap_diagnostic["related_locations"]
        assert len(related) == 1, bootstrap_diagnostic
        related_span = related[0]["span"]
        assert (related_span["start_line"], related_span["start_column"]) == (0, 9)

    typo_source = "proc main() -> int { let user_count = 1; return user_coutn; }\n"
    with tempfile.TemporaryDirectory(prefix="vitte-name-help-parity-") as directory:
        path = Path(directory) / "parity_typo.vit"
        path.write_text(typo_source, encoding="utf-8")
        outputs = []
        for executable in (format_binary, binary):
            document = run_diagnostic(path, "json", executable)
            diagnostic = first_json_diagnostic(document, "E0401")
            terminal = run_diagnostic(path, "terminal", executable)
            outputs.append((diagnostic, terminal))

        bootstrap_diagnostic, bootstrap_terminal = outputs[0]
        native_diagnostic, native_terminal = outputs[1]
        assert bootstrap_diagnostic["code"] == native_diagnostic["code"] == "E0401"
        assert bootstrap_diagnostic["internal_code"] == native_diagnostic["internal_code"]
        assert bootstrap_diagnostic["severity"] == native_diagnostic["severity"] == "error"
        assert bootstrap_diagnostic["message"] == native_diagnostic["message"] == (
            "unknown symbol `user_coutn`"
        )
        assert phase(bootstrap_diagnostic) == phase(native_diagnostic) == "name-resolution"
        bootstrap_help = next(
            item["message"] for item in bootstrap_diagnostic["annotations"]
            if item["kind"] == "help"
        )
        assert bootstrap_help == native_diagnostic["help"]
        assert "Did you mean `user_count` instead of `user_coutn`?" in bootstrap_help
        bootstrap_position = bootstrap_diagnostic["position"]
        native_location = native_diagnostic["location"]
        assert (
            bootstrap_position["start_line"], bootstrap_position["start_column"],
            bootstrap_position["end_line"], bootstrap_position["end_column"],
        ) == (
            native_location["start"]["line"], native_location["start"]["column"],
            native_location["end"]["line"], native_location["end"]["column"],
        )
        for terminal in (bootstrap_terminal, native_terminal):
            assert "category: name-resolution" in terminal
            assert bootstrap_help in terminal
            assert "parity_typo.vit:1:49" in terminal


check_explain_for_every_public_code()
check_format_parity()
check_frontend_user_information_parity()
print("Diagnostic parity: bootstrap/native user information, explain, terminal, JSON, SARIF and LSP passed.")
