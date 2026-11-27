"""Exercise contextual help through the real compiler and structured formats."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

binary = Path(sys.argv[1]).resolve()
diagnostic_binary = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else binary
root = Path(__file__).resolve().parents[2]
version = (root / "VERSION").read_text().strip()
assert subprocess.check_output([binary, "--version"], text=True).splitlines()[0] == f"vitte {version}"


def check_registry_guidance():
    registry = (root / "src" / "bootstrap" / "diagnostic" / "registry.c").read_text()
    codes = list(dict.fromkeys(re.findall(r'"([EWNH][0-9]{4})"', registry)))
    assert len(codes) == 277, ("public diagnostic inventory changed", len(codes))
    guidance_by_code = {}
    valid_severities = {"error", "fatal", "warning", "note", "help"}
    for code in codes:
        result = subprocess.run(
            [str(binary), "explain", code, "--diagnostic=json"],
            capture_output=True,
            text=True,
            timeout=20,
        )
        assert result.returncode == 0 and not result.stderr, (code, result.returncode, result.stderr)
        entry = json.loads(result.stdout)["diagnostic"]
        assert entry["code"] == code, (code, entry)
        for field in (
            "internal", "title", "explanation", "severity", "phase",
            "category", "guidance",
        ):
            assert isinstance(entry.get(field), str) and entry[field].strip(), (code, field, entry)
        assert entry["title"].strip() != entry["explanation"].strip(), (code, entry)
        assert entry["guidance"].strip() not in {
            entry["title"].strip(), entry["explanation"].strip(),
        }, (code, entry)
        assert len(entry["guidance"].strip()) >= 12, (code, entry)
        assert entry["severity"] in valid_severities, (code, entry)
        assert isinstance(entry.get("deprecated"), bool), (code, entry)
        if entry["severity"] in {"error", "fatal", "warning"}:
            prior = guidance_by_code.get(entry["guidance"].strip())
            assert prior is None, ("user-facing guidance must be code-specific", prior, code)
            guidance_by_code[entry["guidance"].strip()] = code


check_registry_guidance()

explanation = json.loads(subprocess.check_output(
    [binary, "explain", "E1503", "--diagnostic=json"], text=True))
assert "divisor" in explanation["diagnostic"]["guidance"]
assert "zero" in explanation["diagnostic"]["guidance"]
assert "divisor" in subprocess.check_output([binary, "explain", "E1503"], text=True)
remainder_explanation = json.loads(subprocess.check_output(
    [binary, "explain", "E1506", "--diagnostic=json"], text=True))
assert "remainder" in remainder_explanation["diagnostic"]["guidance"]
overflow_explanation = json.loads(subprocess.check_output(
    [binary, "explain", "E1502", "--diagnostic=json"], text=True))
assert "overflows `i64`" in overflow_explanation["diagnostic"]["guidance"]
pattern_explanation = json.loads(subprocess.check_output(
    [binary, "explain", "E0284", "--diagnostic=json"], text=True))
assert "`Path::Variant`" in pattern_explanation["diagnostic"]["guidance"]
range_explanation = json.loads(subprocess.check_output(
    [binary, "explain", "E0285", "--diagnostic=json"], text=True))
assert "same kind" in range_explanation["diagnostic"]["guidance"]
invalid = subprocess.run([binary, "explain", "E9999", "--diagnostic=json"],
                         capture_output=True, text=True)
assert invalid.returncode == 2
assert "unknown diagnostic code" in invalid.stderr


def run(path, fmt="json", command="compile"):
    args = [str(diagnostic_binary), command, str(path), "--quiet", "--no-color", f"--diagnostic={fmt}"]
    if command == "compile":
        args.append("--stop-after-sema")
    result = subprocess.run(args, capture_output=True, text=True, timeout=20)
    assert result.returncode == 1, (args, result.returncode, result.stdout, result.stderr)
    if fmt == "terminal":
        return result.stderr
    return json.loads(result.stderr)


def help_for(document, code):
    items = [d for d in document["diagnostics"] if d["code"] == code]
    assert items, document
    assert items[0]["severity"] == "error", items[0]
    expected_phase = {
        "E0005": "io", "E0102": "lexer", "E0201": "parser",
        "E0224": "parser", "E0225": "parser", "E0226": "parser",
        "E0227": "parser", "E0228": "parser", "E0229": "parser",
        "E0230": "parser",
        "E0231": "parser", "E0232": "parser", "E0233": "parser",
        "E0234": "parser", "E0235": "parser", "E0236": "parser",
        "E0237": "parser", "E0238": "parser", "E0239": "parser",
        "E0240": "parser", "E0241": "parser", "E0242": "parser",
        "E0271": "parser", "E0272": "parser", "E0273": "parser", "E0274": "parser",
        "E0275": "parser", "E0276": "parser", "E0502": "type-check",
        "E0277": "parser", "E0278": "parser", "E0279": "parser",
        "E0280": "parser", "E0281": "parser", "E0282": "parser",
        "E0283": "parser", "E0284": "parser", "E0285": "parser",
        "E0286": "parser", "E0287": "parser", "E0288": "parser",
        "E0289": "parser", "E0290": "parser", "E0291": "parser",
        "E0292": "parser", "E0293": "parser", "E0294": "parser",
        "E0295": "parser", "E0296": "parser", "E0297": "parser",
        "E0298": "parser", "E0299": "parser",
        "E0308": "parser", "E0309": "parser", "E0310": "parser",
        "E0311": "parser",
        "E0401": "name-resolution", "E0601": "type-check",
        "E0602": "type-check", "E0605": "type-check",
    }[code]
    assert items[0]["phase"] == expected_phase, items[0]
    helps = [a["message"] for a in items[0]["annotations"] if a["kind"] == "help"]
    assert len(helps) == 1, items[0]
    return helps[0]


with tempfile.TemporaryDirectory(prefix="vitte-help-") as directory:
    path = Path(directory) / "case.vit"

    # The code-only explanation must retain the actionable remediation used
    # by normal diagnostics, while those diagnostics may add operator/value
    # details from the actual source expression.
    for code, source, explain_document, remediation in (
        ("E1502", "proc main() -> int { return 9223372036854775807 + 1; }\n",
         overflow_explanation, "Reduce the operands or use a wider representable type."),
        ("E1503", "proc main() -> int { return 8 / (2 - 2); }\n",
         explanation, "Change the divisor or guard the operation."),
        ("E1506", "proc main() -> int { return 8 % 0; }\n",
         remainder_explanation, "Change the divisor or guard the operation."),
    ):
        path.write_text(source)
        constant_document = run(path)
        constant_diagnostic = next(
            d for d in constant_document["diagnostics"] if d["code"] == code)
        constant_help = next(
            a["message"] for a in constant_diagnostic["annotations"]
            if a["kind"] == "help")
        assert remediation in explain_document["diagnostic"]["guidance"], (
            code, explain_document)
        assert remediation in constant_help, (code, constant_help)

    # Multiple diagnostics from one semantic phase remain one valid document
    # in both JSON and SARIF, with matching code, phase, span, related location
    # and help fields.
    path.write_text(
        "proc add_one(value: int) -> int { return value + 1; }\n"
        "proc main() -> int { return add_one(\"nope\") + add_one(true); }\n"
    )
    multi_json = run(path)
    json_diagnostics = multi_json["diagnostics"]
    assert len(json_diagnostics) == 2, json_diagnostics
    assert {d["code"] for d in json_diagnostics} == {"E0602"}, json_diagnostics
    assert {d["phase"] for d in json_diagnostics} == {"type-check"}, json_diagnostics

    multi_sarif = run(path, "sarif")
    sarif_results = multi_sarif["runs"][0]["results"]
    assert len(sarif_results) == 2, sarif_results
    multi_terminal = run(path, "terminal")
    multi_lsp = run(path, "lsp")
    for diagnostic, result in zip(json_diagnostics, sarif_results):
        region = result["locations"][0]["physicalLocation"]["region"]
        label = next(
            item for item in diagnostic["labels"] if item["style"] == "primary")
        help_text = next(
            item["message"] for item in diagnostic["annotations"]
            if item["kind"] == "help")
        assert diagnostic["code"] == result["ruleId"] == "E0602", result
        assert diagnostic["phase"] == result["properties"]["phase"] == "type-check", result
        assert result["properties"]["help"] == help_text, result
        assert region["byteOffset"] == label["span"]["start_byte"], result
        assert region["byteLength"] == (
            label["span"]["end_byte"] - label["span"]["start_byte"]
        ), result
        assert len(result["relatedLocations"]) == len(diagnostic["related_locations"]), result
        lsp_diagnostic = next(
            item for item in multi_lsp if item["code"] == diagnostic["code"] and
            item["data"]["position"]["startByte"] == diagnostic["position"]["start_byte"]
        )
        assert lsp_diagnostic["data"]["help"] == help_text, lsp_diagnostic
        assert help_text in multi_terminal, multi_terminal

    def case(source, code, expected, absent=None):
        path.write_text(source)
        document = run(path)
        diagnostic = next(
            d for d in document["diagnostics"] if d["code"] == code)
        help_text = help_for(document, code)
        assert expected in help_text, help_text
        if absent:
            assert absent not in help_text, help_text
        assert help_text in run(path, "terminal"), help_text
        sarif = run(path, "sarif")
        assert sarif["runs"][0]["tool"]["driver"]["version"] == version
        result = next(r for r in sarif["runs"][0]["results"] if r["ruleId"] == code)
        assert result["properties"]["help"] == help_text, result
        assert result["level"] == "error", result
        properties = result["properties"]
        assert properties["internalCode"] == diagnostic["internal_code"], result
        assert properties["severity"] == diagnostic["severity"], result
        assert properties["phase"] == diagnostic["phase"] == (
            "name-resolution" if code == "E0401" else
            "parser" if code.startswith("E02") or code in {
                "E0308", "E0309", "E0310", "E0311"
            } else "type-check"), result
        assert properties["category"] == diagnostic["category"], result
        context = diagnostic["context"]
        assert properties.get("expectedType", "") == context["expected_type"], result
        assert properties.get("actualType", "") == context["actual_type"], result
        assert properties.get("expected", "") == context["expected"], result
        assert properties.get("found", "") == context["found"], result
        lsp_diagnostic = next(
            item for item in run(path, "lsp") if item["code"] == code and
            item["data"]["position"]["startByte"] == diagnostic["position"]["start_byte"]
        )
        lsp_data = lsp_diagnostic["data"]
        assert lsp_data["internalCode"] == diagnostic["internal_code"], lsp_diagnostic
        assert lsp_data["severity"] == diagnostic["severity"], lsp_diagnostic
        assert lsp_data["phase"] == diagnostic["phase"], lsp_diagnostic
        assert lsp_data["category"] == diagnostic["category"], lsp_diagnostic
        assert lsp_data["expectedType"] == context["expected_type"], lsp_diagnostic
        assert lsp_data["actualType"] == context["actual_type"], lsp_diagnostic
        assert lsp_data["expected"] == context["expected"], lsp_diagnostic
        assert lsp_data["found"] == context["found"], lsp_diagnostic
        if code == "E0602":
            assert diagnostic["context"]["expected_type"] == "i64", diagnostic
            assert diagnostic["context"]["actual_type"] == "string", diagnostic
            assert properties["expectedType"] == "i64", result
            assert properties["actualType"] == "string", result
            assert lsp_data["expectedType"] == "i64", lsp_diagnostic
            assert lsp_data["actualType"] == "string", lsp_diagnostic
            terminal = run(path, "terminal")
            assert "expected i64, found string" in terminal, terminal
        return help_text

    case("proc main() -> int { let user_count = 2; return user_coutn; }",
         "E0401", "Did you mean `user_count` instead of `user_coutn`?")
    # Deletion, insertion, substitution, and transposition all stay advisory.
    for typo in ("user_coun", "user_countt", "user_coumt"):
        case(f"proc main() -> int {{ let user_count = 2; return {typo}; }}",
             "E0401", "Did you mean `user_count`")
    case("proc main() -> int { let count = 2; let court = 3; return cout; }",
         "E0401", "`cout`", "Did you mean")
    case("proc helper() -> int { let user_count = 2; return user_count; }\n"
         "proc main() -> int { return user_coutn; }",
         "E0401", "`user_coutn`", "Did you mean")
    case("proc main() -> int { let ab = 2; return ac; }",
         "E0401", "`ac`", "Did you mean")
    case("proc add(a: int, b: int) -> int { return a + b; }\n"
         "proc main() -> int { return add(1); }",
         "E0601", "requires 2 argument(s), but 1 were supplied. Add 1")
    case("proc add(a: int, b: int) -> int { return a + b; }\n"
         "proc main() -> int { return add(1, 2, 3); }",
         "E0601", "requires 2 argument(s), but 3 were supplied. Remove 1")
    case("proc main() -> int { return len(); }",
         "E0601", "requires 1 argument(s), but 0 were supplied. Add 1")
    case('proc add(a: int) -> int { return a; }\nproc main() -> int { return add("42"); }',
         "E0602", "Argument 1 does not match parameter `a` of procedure `add`.")
    case('proc main() -> int { return "nope"; }',
         "E0605", "Return value in procedure `main` does not match its declared return type")
    case("const = 1;", "E0224", "Give the declaration a name")
    case("const LIMIT = ;", "E0225", "Add `= expression`")
    case("type Alias = ;", "E0226", "Complete the type name")
    case("opaque type Handle: Trait;", "E0227", "omit bounds")
    case("form Point { value int }", "E0228", "fields in the form `name: Type`")
    case("form Point { value: int, value: bool }", "E0229", "unique name")
    case("form Point { x: int y: int }", "E0230", "Separate adjacent fields with `,` or `;`")
    case("pick { A }", "E0231", "named pick with braces")
    case("pick Choice { Some, Some }", "E0232", "unique within its pick")
    case("pick Choice { Some(,) }", "E0233", "comma-separated types")
    case("trait Display", "E0234", "`trait Name { ... }`")
    case("impl Point", "E0235", "`impl Trait for Type { ... }`")
    case("impl Display for {}", "E0236", "type after `for`")
    case("impl Display for Point extra {}", "E0237", "one trait type, `for`, one target type")
    case("proc () {}", "E0238", "`proc name(parameters)`")
    case("proc f(x int) {}", "E0239", "pattern, `:`, a type")
    case("proc f(x: int {}", "E0239", "closing `)`")
    case("proc f() -> {}", "E0240", "valid type after `->`")
    case("async async proc f() {}", "E0241", "at most once")
    case("async form Point {}", "E0241", "only before a procedure declaration")
    case("proc f() return 1;", "E0242", "block or `;`")
    case("extern proc f() {}", "E0242", "must end with `;`")
    case("extern proc f()", "E0242", "must end with `;`")
    case("proc bad() { let value = ; }", "E0272", "Write `let [mut] pattern")
    case("proc bad() { set value = ; }", "E0273", "complete target, value")
    case("proc bad() { set value == 1; }", "E0274", "Use one of `=`")
    case("proc bad() { return 1 2; }", "E0275", "return expression;")
    case("proc bad() { break value; }", "E0276", "Write `break;` or `continue;`")
    case("proc bad() { defer; }", "E0277", "`defer { ... }` or `defer expression;`")
    case("proc bad() { assert(true; }", "E0278", "`assert(condition);` or `assert condition;`")
    case("proc bad() { unsafe value; }", "E0279", "`unsafe { ... }`")
    case("proc bad() { asm; }", "E0280", "`asm(operand, ...);`")
    case("proc bad() { if; }", "E0281", "if condition { ... }")
    case("proc bad() { match; }", "E0282", "`match value { pattern => body, ... }`")
    case("proc bad() { match true { true false => 0, } }",
         "E0283", "`[case] pattern [if guard] => body`")
    case("proc bad() { match true { (true,, false) => 1, } }",
         "E0284", "`Path::Variant`")
    case("proc bad() { match value { 1..false => 0, } }",
         "E0285", "`lower..=upper`")
    case("proc bad() { let value = target = ; }", "E0286", "`target = value`")
    case("proc bad() { let value = target ?? ; }", "E0287", "`value ?? fallback`")
    case("proc bad() { let value = target and ; }", "E0288", "`and`, `or`, `&&` or `||`")
    case("proc bad() { let value = target | ; }", "E0289", "`&`, `|` or `^`")
    case("proc bad() { let value = target == ; }", "E0290", "`==`, `!=`, `<`, `<=`, `>` or `>=`")
    case("proc bad() { let value = target << ; }", "E0291", "`<<` or `>>`")
    case("proc bad() { let value = target + ; }", "E0292", "`+`, `-`, `*`, `/` or `%`")
    case("proc bad() { let value = target .. ; }", "E0293", "`start..end`")
    case("proc bad() { let value = -; }", "E0294", "one complete operand")
    case("proc bad() { let value = target as ; }", "E0295", "`value as Type`")
    case("type Bad = Map<int bool>;", "E0271", "comma-separated types")
    case("proc bad() { let value = invoke(1; }", "E0296", "`callee(arguments)`")
    case("proc bad() { let value = if ready { 1 }; }", "E0297", "else branch")
    case("proc bad() { let value = { 1 2 }; }", "E0298", "`}`")
    case("proc bad() { let value = [1 2]; }", "E0311", "separated by commas")
    case("const VALUES = [1", "E0299", "close the array with `]`")
    case("proc bad() { let value = sizeof(); }", "E0308", "`sizeof(Type)`")
    case("proc bad() { let value = alignof(); }", "E0309", "`alignof(Type)`")
    case("proc bad() { let value = null(1; }", "E0310", "close the argument list")
    case("proc bad() { let value = invoke(,); }", "E0311", "empty items are invalid")
    case("proc bad() { let value; }", "E0502", "type annotation after this binding")
    path.write_text("proc main() -> int { return (1 + 2; }")
    return_diagnostics = run(path, command="check")
    syntax = help_for(return_diagnostics, "E0275")
    assert "return expression;" in syntax, syntax
    return_diagnostic = next(
        d for d in return_diagnostics["diagnostics"] if d["code"] == "E0275")
    assert return_diagnostic["context"]["expected"] == ")", return_diagnostic
    assert any("opening `(`" in related["label"]
               for related in return_diagnostic["related_locations"]), return_diagnostic
    unterminated_source = 'proc main() -> int { let value = "unterminated;\n }'
    path.write_text(unterminated_source)
    unterminated_document = run(path, command="check")
    unterminated = next(
        d for d in unterminated_document["diagnostics"] if d["code"] == "E0102")
    lexical = help_for(unterminated_document, "E0102")
    assert "Close the string opened" in lexical, lexical
    string_suggestion = unterminated["suggestions"][0]
    string_insert = len(unterminated_source[:unterminated_source.index("\n")].encode("utf-8"))
    assert string_suggestion["applicability"] == "machine-applicable", unterminated
    assert string_suggestion["replacement"] == '"', unterminated
    assert string_suggestion["span"]["source"] == unterminated["position"]["source"] == str(path), unterminated
    assert (string_suggestion["span"]["start_byte"], string_suggestion["span"]["end_byte"]) == (
        string_insert, string_insert), unterminated
    string_sarif = next(
        r for r in run(path, "sarif", command="check")["runs"][0]["results"]
        if r["ruleId"] == "E0102")
    string_fix = string_sarif["fixes"][0]
    string_replacement = string_fix["artifactChanges"][0]["replacements"][0]
    assert string_fix["properties"]["applicability"] == "machine-applicable", string_fix
    assert string_replacement["deletedRegion"]["byteOffset"] == string_insert, string_fix
    assert string_replacement["deletedRegion"]["byteLength"] == 0, string_fix
    assert string_replacement["insertedContent"]["text"] == '"', string_fix
    assert string_fix["artifactChanges"][0]["artifactLocation"]["uri"] == str(path), string_fix
    string_lsp = next(
        d for d in run(path, "lsp", command="check") if d["code"] == "E0102")
    string_lsp_suggestion = string_lsp["data"]["suggestions"][0]
    assert string_lsp["data"]["position"]["source"] == str(path), string_lsp
    assert string_lsp_suggestion["applicability"] == "machine-applicable", string_lsp
    assert (string_lsp_suggestion["startByte"], string_lsp_suggestion["endByte"]) == (
        string_insert, string_insert), string_lsp
    string_terminal = run(path, "terminal", command="check")
    assert "[machine-applicable]" in string_terminal, string_terminal
    assert "Insert the missing closing `\"`" in string_terminal, string_terminal
    # The current grammar accepts the symbolic boolean operators directly.
    path.write_text("proc main() -> int { let value = (1; return value; }")
    punctuation_document = run(path, command="check")
    punctuation = punctuation_document["diagnostics"][0]
    punctuation_label = punctuation["labels"][0]
    punctuation_related = punctuation["related_locations"][0]
    punctuation_help = next(
        a["message"] for a in punctuation["annotations"] if a["kind"] == "help")
    assert punctuation["context"]["expected"] == ")", punctuation
    assert punctuation["context"]["found"] == ";", punctuation
    assert "opening `(`" in punctuation_related["label"], punctuation
    assert punctuation_related["span"]["start_byte"] == path.read_text().index("(", 33), punctuation
    punctuation_sarif = run(path, "sarif", command="check")["runs"][0]["results"][0]
    punctuation_region = punctuation_sarif["locations"][0]["physicalLocation"]["region"]
    punctuation_sarif_related = punctuation_sarif["relatedLocations"][0]
    assert punctuation_region["byteOffset"] == punctuation_label["span"]["start_byte"], punctuation_sarif
    assert punctuation_region["byteLength"] == (
        punctuation_label["span"]["end_byte"] - punctuation_label["span"]["start_byte"]
    ), punctuation_sarif
    assert punctuation_sarif_related["physicalLocation"]["region"]["byteOffset"] == (
        punctuation_related["span"]["start_byte"]), punctuation_sarif
    assert punctuation_sarif["properties"]["help"] == punctuation_help, punctuation_sarif
    punctuation_lsp = run(path, "lsp", command="check")[0]
    assert punctuation_lsp["range"]["start"]["character"] == (
        punctuation_label["span"]["start_column"]), punctuation_lsp
    assert punctuation_lsp["data"]["help"] == punctuation_help, punctuation_lsp
    assert punctuation_lsp["relatedInformation"][0]["message"] == (
        punctuation_related["label"]), punctuation_lsp
    punctuation_terminal = run(path, "terminal", command="check")
    assert punctuation_label["message"] in punctuation_terminal, punctuation_terminal
    assert punctuation_related["label"] in punctuation_terminal, punctuation_terminal
    assert punctuation_help in punctuation_terminal, punctuation_terminal
    path.write_text("proc main() -> int { let value = invoke(1 2); return 0; }")
    missing_comma = run(path, command="check")["diagnostics"][0]
    assert missing_comma["code"] == "E0311", missing_comma
    assert len(missing_comma["suggestions"]) == 1, missing_comma
    suggestion = missing_comma["suggestions"][0]
    assert suggestion["replacement"] == ",", missing_comma
    assert suggestion["applicability"] == "maybe-incorrect", missing_comma
    assert suggestion["span"]["start_byte"] == suggestion["span"]["end_byte"] == path.read_text().index("2"), missing_comma
    assert suggestion["span"]["source"] == missing_comma["position"]["source"] == str(path), missing_comma
    missing_comma_sarif = run(path, "sarif", command="check")["runs"][0]["results"][0]
    assert missing_comma_sarif["ruleId"] == missing_comma["code"] == "E0311", missing_comma_sarif
    assert missing_comma_sarif["properties"]["severity"] == missing_comma["severity"], missing_comma_sarif
    assert missing_comma_sarif["properties"]["phase"] == missing_comma["phase"] == "parser", missing_comma_sarif
    assert missing_comma_sarif["properties"]["category"] == missing_comma["category"] == "syntax", missing_comma_sarif
    assert missing_comma_sarif["properties"]["expected"] == missing_comma["context"]["expected"], missing_comma_sarif
    assert missing_comma_sarif["properties"]["found"] == missing_comma["context"]["found"], missing_comma_sarif
    assert missing_comma_sarif["fixes"][0]["properties"]["applicability"] == (
        suggestion["applicability"]), missing_comma_sarif
    comma_replacement = missing_comma_sarif["fixes"][0]["artifactChanges"][0]["replacements"][0]
    assert comma_replacement["deletedRegion"]["byteOffset"] == suggestion["span"]["start_byte"], missing_comma_sarif
    assert comma_replacement["deletedRegion"]["byteLength"] == 0, missing_comma_sarif
    assert comma_replacement["insertedContent"]["text"] == suggestion["replacement"], missing_comma_sarif
    assert missing_comma_sarif["fixes"][0]["artifactChanges"][0]["artifactLocation"]["uri"] == str(path), missing_comma_sarif
    missing_comma_lsp = run(path, "lsp", command="check")[0]
    assert missing_comma_lsp["code"] == missing_comma["code"], missing_comma_lsp
    assert missing_comma_lsp["data"]["severity"] == missing_comma["severity"], missing_comma_lsp
    assert missing_comma_lsp["data"]["phase"] == missing_comma["phase"], missing_comma_lsp
    assert missing_comma_lsp["data"]["category"] == missing_comma["category"], missing_comma_lsp
    assert missing_comma_lsp["data"]["expected"] == missing_comma["context"]["expected"], missing_comma_lsp
    assert missing_comma_lsp["data"]["found"] == missing_comma["context"]["found"], missing_comma_lsp
    assert missing_comma_lsp["data"]["suggestions"][0]["applicability"] == (
        suggestion["applicability"]), missing_comma_lsp
    assert missing_comma_lsp["data"]["position"]["source"] == str(path), missing_comma_lsp
    assert (missing_comma_lsp["data"]["suggestions"][0]["startByte"],
            missing_comma_lsp["data"]["suggestions"][0]["endByte"]) == (
                suggestion["span"]["start_byte"], suggestion["span"]["end_byte"]), missing_comma_lsp
    missing_comma_terminal = run(path, "terminal", command="check")
    assert suggestion["message"] in missing_comma_terminal, missing_comma_terminal
    assert "[maybe-incorrect]" in missing_comma_terminal, missing_comma_terminal
    path.write_text("proc main() -> int { let value = invoke(,); return 0; }")
    empty_item = run(path, command="check")["diagnostics"][0]
    assert empty_item["code"] == "E0311", empty_item
    assert not empty_item["suggestions"], empty_item
    path.write_text('proc main() -> int { let value = "bad\\q"; }')
    invalid_escape_json = run(path, command="check")
    invalid_escape = next(
        d for d in invalid_escape_json["diagnostics"] if d["code"] == "E0103")
    assert invalid_escape["details"].startswith("invalid escape sequence `\\q`"), invalid_escape
    assert "accepted forms:" in invalid_escape["details"], invalid_escape
    assert invalid_escape["position"]["end_byte"] == (
        invalid_escape["position"]["start_byte"] + 2), invalid_escape
    invalid_escape_terminal = run(path, "terminal", command="check")
    assert "invalid escape sequence `\\q`" in invalid_escape_terminal, invalid_escape_terminal
    invalid_escape_sarif = run(path, "sarif", command="check")
    invalid_escape_result = next(
        r for r in invalid_escape_sarif["runs"][0]["results"]
        if r["ruleId"] == "E0103")
    assert invalid_escape_result["locations"][0]["message"]["text"].startswith(
        "invalid escape sequence `\\q`"), invalid_escape_result
    assert invalid_escape_result["properties"]["phase"] == "lexer", invalid_escape_result

    character_source = "proc main() -> int { let value = 'a; }"
    path.write_text(character_source)
    invalid_character_json = run(path, command="check")
    invalid_character = next(
        d for d in invalid_character_json["diagnostics"] if d["code"] == "E0111")
    assert invalid_character["details"].startswith("malformed character literal"), invalid_character
    assert invalid_character["suggestions"], invalid_character
    character_suggestion = invalid_character["suggestions"][0]
    character_insert = character_source.index(";")
    assert character_suggestion["replacement"] == "'", invalid_character
    assert character_suggestion["applicability"] == "manual", invalid_character
    assert character_suggestion["span"]["source"] == invalid_character["position"]["source"] == str(path), invalid_character
    assert (character_suggestion["span"]["start_byte"], character_suggestion["span"]["end_byte"]) == (
        character_insert, character_insert), invalid_character
    invalid_character_sarif = run(path, "sarif", command="check")
    invalid_character_result = next(
        r for r in invalid_character_sarif["runs"][0]["results"]
        if r["ruleId"] == "E0111")
    assert invalid_character_result["fixes"], invalid_character_result
    assert invalid_character_result["fixes"][0]["artifactChanges"], invalid_character_result
    character_fix = invalid_character_result["fixes"][0]
    character_replacement = character_fix["artifactChanges"][0]["replacements"][0]
    assert character_fix["properties"]["applicability"] == "manual", character_fix
    assert character_replacement["deletedRegion"]["byteOffset"] == character_insert, character_fix
    assert character_replacement["deletedRegion"]["byteLength"] == 0, character_fix
    assert character_replacement["insertedContent"]["text"] == "'", character_fix
    assert character_fix["artifactChanges"][0]["artifactLocation"]["uri"] == str(path), character_fix
    character_lsp = next(
        d for d in run(path, "lsp", command="check") if d["code"] == "E0111")
    character_lsp_suggestion = character_lsp["data"]["suggestions"][0]
    assert character_lsp["data"]["position"]["source"] == str(path), character_lsp
    assert character_lsp_suggestion["applicability"] == "manual", character_lsp
    assert (character_lsp_suggestion["startByte"], character_lsp_suggestion["endByte"]) == (
        character_insert, character_insert), character_lsp
    character_terminal = run(path, "terminal", command="check")
    assert "[manual]" in character_terminal, character_terminal
    assert "insert a closing `'` after this one ASCII character" in character_terminal, character_terminal

    path.write_text("proc main() -> int { let value = 'ab; }")
    ambiguous_character = next(
        d for d in run(path, command="check")["diagnostics"] if d["code"] == "E0111")
    assert not ambiguous_character["suggestions"], ambiguous_character

    path.write_text("use sample.lib;\nproc main() -> int { return 1; }\n")
    import_path = next(d for d in run(path, command="check")["diagnostics"]
                       if d["code"] == "E0212")
    assert "separated by `::` or `/`" in import_path["details"], import_path

    path.write_text(
        "use sample::lib::public_value;\n"
        "proc main() -> int { return 0; }\n")
    symbol_import = next(
        d for d in run(path, command="check")["diagnostics"]
        if d["code"] == "E0301")
    symbol_import_help = next(
        a["message"] for a in symbol_import["annotations"]
        if a["kind"] == "help")
    assert "search roots checked" in symbol_import["details"], symbol_import
    assert "Check the exact module path" in symbol_import_help, symbol_import_help

    missing = Path(directory) / "missing.vit"
    source_help = help_for(run(missing, command="check"), "E0005")
    assert str(missing) in source_help, source_help

    module_dir = Path(directory) / "imports" / "sample"
    module_dir.mkdir(parents=True)
    library = module_dir / "lib.vit"
    main = module_dir.parent / "main.vit"
    library.write_text(
        "space sample::lib;\n"
        "const private_value: i64 = 1;\n"
        "export const public_value: i64 = 2;\n")
    main.write_text(
        "space sample::main;\n"
        "use sample::lib;\n"
        "proc main() -> i64 { return lib::private_value; }\n")
    import_json = run(main)
    import_diagnostic = next(
        d for d in import_json["diagnostics"] if d["code"] == "E0404")
    import_help = next(
        a["message"] for a in import_diagnostic["annotations"]
        if a["kind"] == "help")
    assert "private_value" in import_help, import_help
    assert "pub" in import_help and "export" in import_help, import_help
    assert import_diagnostic["related_locations"], import_diagnostic
    assert import_diagnostic["related_locations"][0]["span"]["source"] == str(library), import_diagnostic
    import_terminal = run(main, "terminal")
    assert import_help in import_terminal, import_terminal
    assert str(library) in import_terminal, import_terminal
    import_sarif = run(main, "sarif")
    import_result = next(
        r for r in import_sarif["runs"][0]["results"] if r["ruleId"] == "E0404")
    assert import_result["properties"]["help"] == import_help, import_result
    assert import_result["relatedLocations"], import_result
    assert import_result["relatedLocations"][0]["physicalLocation"][
        "artifactLocation"]["uri"] == str(library), import_result

    collision = module_dir.parent / "collision.vit"
    collision.write_text(
        "space sample::collision;\n"
        "const public_value: i64 = 1;\n"
        "use sample::lib::*;\n"
        "proc main() -> i64 { return public_value; }\n")
    collision_json = run(collision)
    collision_diagnostic = next(
        d for d in collision_json["diagnostics"] if d["code"] == "E0402")
    assert collision_diagnostic["related_locations"][0]["span"][
        "source"] == str(library), collision_diagnostic
    collision_help = next(
        a["message"] for a in collision_diagnostic["annotations"]
        if a["kind"] == "help")
    assert "public_value" in collision_help, collision_help
    assert collision_help in run(collision, "terminal"), collision_help
    collision_sarif = run(collision, "sarif")
    collision_result = next(
        r for r in collision_sarif["runs"][0]["results"] if r["ruleId"] == "E0402")
    assert collision_result["properties"]["help"] == collision_help, collision_result
    assert collision_result["relatedLocations"][0]["physicalLocation"][
        "artifactLocation"]["uri"] == str(library), collision_result

print("Contextual diagnostic help: CLI, JSON, SARIF and version checks passed.")
