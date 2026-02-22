#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"

usage() {
  echo "usage: $0 <bin1> <bin2>" >&2
  exit 2
}

is_native_vit() {
  local f="$1"
  [[ -f "$f" ]] && rg -q '^space ' "$f" && ! rg -q '^#include ' "$f"
}

generate_entry() {
  local cmd="$1"
  local safe_cmd="${cmd//-/_}"
  local dir="${ROOT}/${cmd}"
  local entry="${dir}/${cmd}.vit"

  mkdir -p "$dir"

  if is_native_vit "$entry"; then
    echo "skip native entry: ${entry}"
    return 0
  fi

  cat > "$entry" <<EOV
<<<
${cmd}.vit
package vitteos/bin/${cmd}
>>>

space vitteos/bin/${cmd}

use vitte/cli as cli

form ParsedArgs {
  args: [string]
  ok: bool
  error: string
}

proc parse_flags(args: [string]) -> ParsedArgs {
  give ParsedArgs(args, true, "")
}

proc ${safe_cmd}_transform(args: [string]) -> [string] {
  let parsed = parse_flags(args)
  if !parsed.ok {
    give ["ERR:" + parsed.error]
  }
  give ["${cmd^^}"]
}

proc command_spec() -> cli.CliSpec {
  give cli.spec("${cmd}", "0.1.0", "TODO: describe ${cmd}")
}

proc usage() -> string {
  let base = cli.usage(command_spec())
  give base + "\\nusage: ${cmd} [args ...]"
}

#[extern]
proc os_argc() -> int

#[extern]
proc os_argv(i: int) -> string

#[extern]
proc os_${safe_cmd}_run(args: [string]) -> int

#[extern]
proc os_write_stdout_line(line: string) -> int

#[extern]
proc os_write_stderr_line(line: string) -> int

proc collect_cli_args() -> [string] {
  let out: [string] = []
  let argc = os_argc()
  let i: int = 1
  loop {
    if i >= argc { break }
    out = out.push(os_argv(i))
    i = i + 1
  }
  give out
}

proc main() -> int {
  let args = collect_cli_args()
  if cli.has_flag(args, "--help") {
    os_write_stdout_line(usage())
    give 0
  }
  if cli.has_flag(args, "--version") {
    os_write_stdout_line("${cmd} 0.1.0")
    give 0
  }

  let parsed = parse_flags(args)
  if !parsed.ok {
    os_write_stderr_line(usage())
    os_write_stderr_line(parsed.error)
    give 1
  }

  give os_${safe_cmd}_run(parsed.args)
}
EOV

  echo "generated entry: ${entry}"
}

generate_tests() {
  local cmd="$1"
  local safe_cmd="${cmd//-/_}"
  local test_dir="${ROOT}/${cmd}/tests"
  local test_file="${test_dir}/${cmd}_transform_tests.vit"

  mkdir -p "$test_dir"

  if is_native_vit "$test_file"; then
    echo "skip native tests: ${test_file}"
    return 0
  fi

  cat > "$test_file" <<EOT
<<<
${cmd}_transform_tests.vit
package vitteos/bin/${cmd}/tests
>>>

space vitteos/bin/${cmd}/tests

use vitte/test as vtest
use vitteos/bin/${cmd} as cmdmod

proc test_transform_non_empty() -> bool {
  let out = cmdmod.${safe_cmd}_transform([])
  give out.len > 0
}

proc status_of(ok: bool) -> vtest.TestStatus {
  if ok { give vtest.TestStatus.Passed }
  give vtest.TestStatus.Failed
}

proc build_suite() -> vtest.TestSuite {
  let suite = vtest.suite("${cmd}_transform")
  suite = vtest.add_case(suite, "test_transform_non_empty", status_of(test_transform_non_empty()), "")
  give suite
}
EOT

  echo "generated tests: ${test_file}"
}

if [[ $# -ne 2 ]]; then
  usage
fi

for cmd in "$1" "$2"; do
  if [[ ! -d "${ROOT}/${cmd}" ]]; then
    echo "missing bin directory: ${ROOT}/${cmd}" >&2
    exit 1
  fi
  generate_entry "$cmd"
  generate_tests "$cmd"
done

echo "running uniformity check"
tools/vitteos_bin_uniformity_check.sh

echo "running vit check"
tools/vitteos_bin_vit_check.sh
