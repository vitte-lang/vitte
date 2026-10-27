"""Targeted bootstrap regressions for imports and contextual diagnostics."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


if len(sys.argv) != 2:
    raise SystemExit(f"usage: {Path(sys.argv[0]).name} VITTE_BINARY")


BINARY = Path(sys.argv[1]).resolve()


def run(path: Path, fmt: str = "json") -> object:
    result = subprocess.run(
        [
            str(BINARY),
            "check",
            str(path),
            "--quiet",
            "--no-color",
            f"--diagnostic={fmt}",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 1:
        raise AssertionError(
            f"unexpected exit status for {path}: {result.returncode}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    if fmt == "terminal":
        return result.stderr
    try:
        return json.loads(result.stderr)
    except json.JSONDecodeError as error:
        raise AssertionError(
            f"invalid {fmt} diagnostics for {path}:\n{result.stderr}"
        ) from error


def diagnostic(document: dict[str, object], code: str) -> dict[str, object]:
    diagnostics = document["diagnostics"]
    assert isinstance(diagnostics, list), document
    matches = [item for item in diagnostics if item.get("code") == code]
    assert len(matches) == 1, (code, document)
    item = matches[0]
    assert isinstance(item, dict), item
    return item


def test_transitive_import_loaded_once(root: Path) -> None:
    package = root / "transitive"
    package.mkdir()
    (package / "root.vit").write_text(
        "space transitive::root;\n"
        "use transitive::left;\n"
        "use transitive::right;\n"
        "proc main() -> int { return left::value() + right::value(); }\n",
        encoding="utf-8",
    )
    (package / "left.vit").write_text(
        "space transitive::left;\n"
        "use transitive::common;\n"
        "export proc value() -> int { return common::VALUE; }\n",
        encoding="utf-8",
    )
    (package / "right.vit").write_text(
        "space transitive::right;\n"
        "use transitive::common;\n"
        "export proc value() -> int { return common::VALUE; }\n",
        encoding="utf-8",
    )
    (package / "common.vit").write_text(
        "space transitive::common;\n"
        "export const VALUE: i64 = \"bad\";\n",
        encoding="utf-8",
    )

    # Both branches reach common.vit. A second expansion would duplicate its
    # declaration and therefore duplicate the single expected type error.
    document = run(package / "root.vit")
    assert isinstance(document, dict)
    diagnostic_item = diagnostic(document, "E0501")
    assert diagnostic_item["details"] == "expected i64, found string"
    diagnostics = document["diagnostics"]
    assert isinstance(diagnostics, list)
    assert len(diagnostics) == 1, document


def test_cycle_chain(root: Path) -> None:
    package = root / "cycle"
    package.mkdir()
    (package / "root.vit").write_text(
        "space cycle::root;\n"
        "use cycle::one;\n"
        "proc main() -> int { return 0; }\n",
        encoding="utf-8",
    )
    (package / "one.vit").write_text(
        "space cycle::one;\nuse cycle::two;\n"
        "export const VALUE: int = 1;\n",
        encoding="utf-8",
    )
    (package / "two.vit").write_text(
        "space cycle::two;\nuse cycle::three;\n"
        "export const VALUE: int = 2;\n",
        encoding="utf-8",
    )
    (package / "three.vit").write_text(
        "space cycle::three;\nuse cycle::one;\n"
        "export const VALUE: int = 3;\n",
        encoding="utf-8",
    )

    document = run(package / "root.vit")
    assert isinstance(document, dict)
    cycle = diagnostic(document, "E0303")
    details = cycle["details"]
    assert isinstance(details, str)
    assert "one.vit ->" in details
    assert "two.vit ->" in details
    assert "three.vit ->" in details
    position = cycle["position"]
    assert position["source"] == str(package / "one.vit"), cycle
    assert position["start_line"] == 1, cycle
    assert position["start_column"] == 0, cycle
    assert cycle["context"]["module"] == "cycle::two", cycle
    assert cycle["context"]["context"] == "use declaration imports `cycle::two`", cycle
    terminal = run(package / "root.vit", "terminal")
    assert isinstance(terminal, str)
    assert details in terminal


def test_import_alias_span(root: Path) -> None:
    package = root / "alias"
    package.mkdir()
    main = package / "main.vit"
    main.write_text(
        "space alias::main;\n"
        "use alias::a as same;\n"
        "use alias::b as same;\n"
        "proc main() -> int { return 0; }\n",
        encoding="utf-8",
    )
    (package / "a.vit").write_text(
        "space alias::a;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    (package / "b.vit").write_text(
        "space alias::b;\nexport const VALUE: i64 = 2;\n",
        encoding="utf-8",
    )

    document = run(main)
    assert isinstance(document, dict)
    conflict = diagnostic(document, "E0304")
    position = conflict["position"]
    assert position["source"] == str(main), conflict
    assert position["start_line"] == 2, conflict
    assert position["start_column"] == 0, conflict
    assert "alias::b" in conflict["details"], conflict
    assert "alias `same`" in conflict["details"], conflict
    assert conflict["context"]["module"] == "alias::b", conflict
    assert conflict["context"]["symbol"] == "same", conflict
    assert conflict["context"]["context"] == "use declaration", conflict

    sarif = run(main, "sarif")
    assert isinstance(sarif, dict)
    result = next(
        item
        for item in sarif["runs"][0]["results"]
        if item["ruleId"] == "E0304"
    )
    location = result["locations"][0]["physicalLocation"]
    assert location["artifactLocation"]["uri"] == str(main), result
    assert location["region"]["startLine"] == 3, result
    assert location["region"]["startColumn"] == 1, result


def test_missing_import_span(root: Path) -> None:
    package = root / "missing"
    package.mkdir()
    main = package / "main.vit"
    main.write_text(
        "space missing::main;\n"
        "use missing::not_found;\n"
        "proc main() -> int { return 0; }\n",
        encoding="utf-8",
    )

    document = run(main)
    assert isinstance(document, dict)
    missing = diagnostic(document, "E0301")
    position = missing["position"]
    assert position["source"] == str(main), missing
    assert position["start_line"] == 1, missing
    assert position["start_column"] == 0, missing
    assert "missing::not_found" in missing["details"], missing
    assert missing["context"]["module"] == "missing::not_found", missing


def test_symlink_path_is_cached_once(root: Path) -> None:
    real = root / "real"
    package = real / "symlinked"
    package.mkdir(parents=True)
    (package / "main.vit").write_text(
        "space symlinked::main;\n"
        "use symlinked::lib;\n"
        "use symlinked::lib;\n"
        "proc main() -> int { return 0; }\n",
        encoding="utf-8",
    )
    (package / "lib.vit").write_text(
        "space symlinked::lib;\n"
        "export const VALUE: i64 = \"bad\";\n",
        encoding="utf-8",
    )
    link = root / "link"
    try:
        link.symlink_to(real, target_is_directory=True)
    except (NotImplementedError, OSError):
        print("[SKIP] symlink regression (symlinks unavailable)")
        return

    document = run(link / "symlinked" / "main.vit")
    assert isinstance(document, dict)
    mismatch = diagnostic(document, "E0501")
    diagnostics = document["diagnostics"]
    assert isinstance(diagnostics, list)
    assert len(diagnostics) == 1, document
    assert mismatch["details"] == "expected i64, found string"


def test_private_symbol_origin(root: Path) -> None:
    package = root / "visibility"
    package.mkdir()
    library = package / "lib.vit"
    main = package / "main.vit"
    library.write_text(
        "space visibility::lib;\n"
        "const SECRET: i64 = 1;\n",
        encoding="utf-8",
    )
    main.write_text(
        "space visibility::main;\n"
        "use visibility::lib;\n"
        "proc main() -> i64 { return lib::SECRET; }\n",
        encoding="utf-8",
    )

    document = run(main)
    assert isinstance(document, dict)
    private = diagnostic(document, "E0404")
    related = private["related_locations"]
    assert isinstance(related, list) and related
    related_span = related[0]["span"]
    assert related_span["source"] == str(library), private

    terminal = run(main, "terminal")
    assert isinstance(terminal, str)
    assert str(library) in terminal
    sarif = run(main, "sarif")
    assert isinstance(sarif, dict)
    result = next(
        item for item in sarif["runs"][0]["results"] if item["ruleId"] == "E0404"
    )
    assert result["relatedLocations"][0]["physicalLocation"]["artifactLocation"][
        "uri"
    ] == str(library), result


TYPE_CASES = {
    "argument": (
        "E0602",
        "space type_argument;\n"
        "proc takes(value: i64) -> i64 { return value; }\n"
        "proc main() -> int { return takes(\"bad\"); }\n",
        "argument 1 / parameter `value`",
        "the argument does not satisfy the parameter type",
        "i64",
        "string",
        True,
    ),
    "field": (
        "E0501",
        "space type_field;\n"
        "form Point { value: i64, }\n"
        "proc main() -> int { let point: Point = Point { value: \"bad\" }; return 0; }\n",
        "field `value` / value",
        "the field value does not satisfy the declared field type",
        "i64",
        "string",
        True,
    ),
    "return": (
        "E0605",
        "space type_return;\n"
        "proc main() -> i64 { return \"bad\"; }\n",
        "return value / declared return type",
        "the returned value does not satisfy the declared return type",
        "i64",
        "string",
        False,
    ),
    "mutability": (
        "E0513",
        "space type_mutability;\n"
        "proc main() -> int { let value: i64 = 1; set value = 2; return 0; }\n",
        "assignment / mutable binding",
        "the assignment target is immutable or not assignable",
        "",
        "",
        True,
    ),
}


def test_type_diagnostics_all_formats(root: Path) -> None:
    for name, (code, source, context, cause, expected, found, has_related) in TYPE_CASES.items():
        path = root / f"{name}.vit"
        path.write_text(source, encoding="utf-8")
        document = run(path)
        assert isinstance(document, dict)
        item = diagnostic(document, code)
        item_context = item["context"]
        assert item_context["context"] == context, item
        assert item_context["probable_cause"] == cause, item
        assert item_context["expected_type"] == expected, item
        assert item_context["actual_type"] == found, item
        help_text = next(
            annotation["message"]
            for annotation in item["annotations"]
            if annotation["kind"] == "help"
        )
        if has_related:
            assert item["related_locations"], item

        terminal = run(path, "terminal")
        assert isinstance(terminal, str)
        assert code in terminal
        assert context in terminal
        assert cause in terminal
        if expected:
            assert f"expected {expected}, found {found}" in terminal
        assert help_text in terminal

        sarif = run(path, "sarif")
        assert isinstance(sarif, dict)
        result = next(
            result
            for result in sarif["runs"][0]["results"]
            if result["ruleId"] == code
        )
        properties = result["properties"]
        assert properties["context"] == context, result
        assert properties["probableCause"] == cause, result
        if expected or found:
            assert properties["expectedType"] == expected, result
            assert properties["actualType"] == found, result
        assert properties["help"] == help_text, result
        if has_related:
            assert result["relatedLocations"], result


def test_diagnostic_format_parity(root: Path) -> None:
    path = root / "format_parity.vit"
    path.write_text(
        "space parity::format;\n"
        "proc main() -> i64 { return \"bad\"; }\n",
        encoding="utf-8",
    )

    document = run(path)
    assert isinstance(document, dict)
    json_item = diagnostic(document, "E0605")
    position = json_item["position"]
    context = json_item["context"]

    terminal = run(path, "terminal")
    assert isinstance(terminal, str)
    assert "E0605" in terminal
    assert json_item["message"] in terminal
    assert json_item["details"] in terminal
    assert context["context"] in terminal
    assert context["probable_cause"] in terminal
    assert next(
        annotation["message"]
        for annotation in json_item["annotations"]
        if annotation["kind"] == "help"
    ) in terminal

    sarif = run(path, "sarif")
    assert isinstance(sarif, dict)
    sarif_item = next(
        item for item in sarif["runs"][0]["results"]
        if item["ruleId"] == "E0605"
    )
    sarif_region = sarif_item["locations"][0]["physicalLocation"]["region"]
    assert sarif_region["startLine"] == position["start_line"] + 1
    assert sarif_region["startColumn"] == position["start_column"] + 1
    assert sarif_item["properties"]["expectedType"] == context["expected_type"]
    assert sarif_item["properties"]["actualType"] == context["actual_type"]
    assert sarif_item["properties"]["context"] == context["context"]
    assert sarif_item["properties"]["probableCause"] == context["probable_cause"]

    lsp = run(path, "lsp")
    assert isinstance(lsp, list)
    lsp_item = next(item for item in lsp if item["code"] == "E0605")
    assert lsp_item["message"] == json_item["message"]
    assert lsp_item["range"]["start"]["line"] == position["start_line"]
    assert lsp_item["range"]["start"]["character"] == position["start_column"]
    assert lsp_item["data"]["expectedType"] == context["expected_type"]
    assert lsp_item["data"]["actualType"] == context["actual_type"]
    assert lsp_item["data"]["context"] == context["context"]
    assert lsp_item["data"]["probableCause"] == context["probable_cause"]
    assert lsp_item["data"]["help"] in terminal


def test_lossy_implicit_conversions_are_rejected(root: Path) -> None:
    path = root / "lossy_conversions.vit"
    path.write_text(
        "space conversion::lossy;\n"
        "form Point { value: f64, }\n"
        "proc takes(value: f64) -> f64 { return value; }\n"
        "proc main() -> f64 {\n"
        "    let integer: i64 = 42;\n"
        "    let point: Point = Point { value: integer };\n"
        "    let mut target: f64 = 0.0;\n"
        "    set target = integer;\n"
        "    takes(integer);\n"
        "    return integer;\n"
        "}\n",
        encoding="utf-8",
    )

    document = run(path)
    assert isinstance(document, dict)
    diagnostics = document["diagnostics"]
    assert isinstance(diagnostics, list)
    mismatches = [
        item for item in diagnostics
        if item["details"] == "expected f64, found i64"
    ]
    assert len(mismatches) == 4, document
    contexts = {item["context"]["context"] for item in mismatches}
    assert contexts == {
        "field `value` / value",
        "assignment / mutable binding",
        "argument 1 / parameter `value`",
        "return value / declared return type",
    }, document


def test_imported_generic_nominal_arguments_are_checked(root: Path) -> None:
    package = root / "generic"
    package.mkdir()
    (package / "lib.vit").write_text(
        "space generic::lib;\n"
        "export form Box<T> { value: T }\n",
        encoding="utf-8",
    )
    main = package / "main.vit"
    main.write_text(
        "space generic::main;\n"
        "use generic::lib as lib;\n"
        "proc main() { let value: lib::Box<Missing>; }\n",
        encoding="utf-8",
    )

    document = run(main)
    assert isinstance(document, dict)
    unknown = diagnostic(document, "E0510")
    assert unknown["labels"][0]["span"]["source"] == str(main), unknown
    assert "Missing" in unknown["annotations"][0]["message"], unknown


with TemporaryDirectory(prefix="vitte-bootstrap-regressions-") as directory:
    workspace = Path(directory)
    test_transitive_import_loaded_once(workspace)
    test_cycle_chain(workspace)
    test_import_alias_span(workspace)
    test_missing_import_span(workspace)
    test_symlink_path_is_cached_once(workspace)
    test_private_symbol_origin(workspace)
    test_type_diagnostics_all_formats(workspace)
    test_diagnostic_format_parity(workspace)
    test_lossy_implicit_conversions_are_rejected(workspace)
    test_imported_generic_nominal_arguments_are_checked(workspace)

print("Bootstrap targeted import and diagnostic regressions passed.")
