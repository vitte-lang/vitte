"""Check multi-error recovery and deterministic diagnostic ordering."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile


executables = (
    ("bootstrap", Path(sys.argv[2]).resolve()),
    ("self-hosted", Path(sys.argv[1]).resolve()),
)

CASES = (
    (
        "independent semantic errors",
        "proc first() -> bool { return not missing_one; }\n"
        "proc second() -> bool { return not missing_two; }\n",
        "name-resolution",
        ("E0401", "E0401"),
    ),
    (
        "independent parser errors",
        "proc first() -> i64 { let first_value = ; return 0; }\n"
        "proc second() -> i64 { let second_value = ; return 0; }\n",
        "parser",
        None,
    ),
)


def location_key(diagnostic):
    if "position" in diagnostic:
        position = diagnostic["position"]
        return position["start_line"], position["start_column"], position["start_byte"]
    location = diagnostic.get("location", {})
    start = location.get("start", {})
    return start.get("line", -1), start.get("column", -1), -1


def public_phase(value):
    return value.replace("_", "-")


def run_once(executable, path, case_name, source, expected_phase):
    command = [
        str(executable), "check", str(path), "--quiet", "--no-color",
        "--diagnostic=json",
    ]
    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        timeout=20,
    )
    assert result.returncode == 1, (
        case_name, executable, result.returncode, result.stdout, result.stderr,
    )
    try:
        document = json.loads(result.stderr)
    except json.JSONDecodeError as error:
        raise AssertionError(
            (case_name, executable, "compiler did not return valid JSON", result.stderr)
        ) from error
    diagnostics = document.get("diagnostics")
    assert isinstance(diagnostics, list), (case_name, executable, document)
    errors = [
        diagnostic for diagnostic in diagnostics
        if diagnostic.get("severity") in ("error", "fatal")
    ]
    assert len(errors) == 2, (
        case_name, executable, "expected exactly two independent errors",
        document,
    )
    assert all(public_phase(item["phase"]) == expected_phase for item in errors), (
        case_name, executable, expected_phase, errors,
    )
    assert all(item["code"].startswith("E") for item in errors), (
        case_name, executable, errors,
    )
    assert all(item["message"].strip() for item in errors), (
        case_name, executable, errors,
    )

    keys = [location_key(item) for item in errors]
    assert keys[0] < keys[1], (case_name, executable, "diagnostics are not source ordered", keys)
    assert not any(
        item.get("code") in {"E0001", "E0002", "E0003", "E0012", "E0013"}
        for item in errors
    ), (case_name, executable, "an internal or phase failure replaced source errors", errors)

    if case_name == "independent semantic errors":
        codes = tuple(item["code"] for item in errors)
        assert codes == ("E0401", "E0401"), (executable, errors)
        rendered_text = "\n".join(
            item.get("message", "") + " " + item.get("details", "") + " " +
            item.get("help", "") + " " + " ".join(
                annotation.get("message", "")
                for annotation in item.get("annotations", [])
            )
            for item in errors
        )
        for name in ("missing_one", "missing_two"):
            assert name in rendered_text, (executable, name, errors)
    else:
        # Each malformed let has its own diagnostic at the missing initializer;
        # recovery must reach the next declaration without fabricating cascades.
        assert len(errors) <= source.count("let ")
        found = []
        for item in errors:
            line, column, byte = location_key(item)
            found.append((line, column, byte))
        assert found[0] != found[1], (executable, errors)

    return tuple(
        (
            item["code"], item["internal_code"], item["severity"],
            public_phase(item["phase"]), item["message"], item.get("details", ""),
            item.get("help", ""), location_key(item),
        )
        for item in errors
    )


with tempfile.TemporaryDirectory(prefix="vitte-diagnostic-recovery-") as temporary:
    directory = Path(temporary)
    for name, source, phase, expected_codes in CASES:
        path = directory / (name.replace(" ", "_") + ".vit")
        path.write_text(source, encoding="utf-8")
        for frontend, executable in executables:
            runs = [
                run_once(executable, path, name, source, phase)
                for _ in range(3)
            ]
            assert runs[0] == runs[1] == runs[2], (
                frontend, name, "diagnostic order/content changed between identical runs", runs,
            )
            if expected_codes is not None:
                assert tuple(item[0] for item in runs[0]) == expected_codes, (
                    frontend, name, runs[0],
                )

print("Diagnostic recovery passed: independent parser and semantic errors are bounded and stable.")
