#!/usr/bin/env bash
set -euo pipefail

ROOT="VitteOS_voyager49/vitte_os_voyager49_kernel/bin"
STATUS_FILE="${ROOT}/MIGRATION_STATUS.md"
CHECKER="/tmp/vit_check"
ONLY_CMD=""

if [[ ! -d "${ROOT}" || ! -f "${STATUS_FILE}" ]]; then
  echo "vitteos-bin-vit-check: skipped (missing ${STATUS_FILE})"
  exit 0
fi

if [[ $# -gt 0 ]]; then
  if [[ $# -eq 2 && "$1" == "--only" ]]; then
    ONLY_CMD="$2"
  else
    echo "usage: $0 [--only <command>]" >&2
    exit 2
  fi
fi

build_checker() {
  g++ -std=c++20 -Wall -Wextra -Werror -O2 -g -Isrc \
    /tmp/vit_check.cpp \
    src/compiler/frontend/lexer.cpp \
    src/compiler/frontend/parser.cpp \
    src/compiler/frontend/diagnostics.cpp \
    src/compiler/frontend/ast.cpp \
    src/compiler/frontend/disambiguate.cpp \
    src/compiler/frontend/lower_hir.cpp \
    src/compiler/ir/hir.cpp \
    -o "${CHECKER}"
}

if [[ ! -x "${CHECKER}" ]]; then
  cat > /tmp/vit_check.cpp <<'CPP'
#include "compiler/frontend/diagnostics.hpp"
#include "compiler/frontend/disambiguate.hpp"
#include "compiler/frontend/lexer.hpp"
#include "compiler/frontend/lower_hir.hpp"
#include "compiler/frontend/parser.hpp"

#include <fstream>
#include <iostream>
#include <string>

using vitte::frontend::diag::DiagnosticEngine;
using vitte::frontend::diag::render_all;

static bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    out.assign((std::istreambuf_iterator<char>(in)),
               std::istreambuf_iterator<char>());
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: vit_check <file.vit> [file.vit ...]\\n";
        return 2;
    }

    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        std::string path = argv[i];
        std::string source;
        if (!read_file(path, source)) {
            std::cerr << path << ": cannot open\\n";
            failed++;
            continue;
        }

        vitte::frontend::Lexer lexer(source, path);
        DiagnosticEngine diagnostics(std::string{});
        vitte::frontend::ast::AstContext ast_ctx;
        vitte::frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, false);

        auto module = parser.parse_module();
        vitte::frontend::passes::disambiguate_invokes(ast_ctx, module);
        if (diagnostics.has_errors()) {
            std::cerr << "[parse] " << path << "\\n";
            render_all(diagnostics, std::cerr);
            failed++;
            continue;
        }

        vitte::ir::HirContext hir_ctx;
        (void)vitte::frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        if (diagnostics.has_errors()) {
            std::cerr << "[lower] " << path << "\\n";
            render_all(diagnostics, std::cerr);
            failed++;
            continue;
        }

        std::cout << "OK " << path << "\\n";
    }

    return failed == 0 ? 0 : 1;
}
CPP
  build_checker
fi

if [[ ! -f "${STATUS_FILE}" ]]; then
  echo "missing status file: ${STATUS_FILE}" >&2
  exit 1
fi

is_native_vit() {
  local f="$1"
  grep -q '^space ' "$f" && ! grep -q '^#include ' "$f"
}

mapfile -t done_cmds < <(awk -F'|' '
  /^\|/ {
    cmd = $2
    st = $3
    gsub(/^[ \t]+|[ \t]+$/, "", cmd)
    gsub(/^[ \t]+|[ \t]+$/, "", st)
    if (cmd != "" && cmd != "command" && st == "done") {
      print cmd
    }
  }
' "${STATUS_FILE}")

candidate_roots=()
if [[ -n "${ONLY_CMD}" ]]; then
  candidate_roots+=("${ROOT}/${ONLY_CMD}")
else
  for cmd in "${done_cmds[@]}"; do
    candidate_roots+=("${ROOT}/${cmd}")
  done
fi

vit_files=()
for base in "${candidate_roots[@]}"; do
  if [[ ! -d "${base}" ]]; then
    continue
  fi
  while IFS= read -r f; do
    if is_native_vit "$f"; then
      vit_files+=("$f")
    fi
  done < <(find "${base}" -type f -name '*.vit' | sort)
done

baseline_root="${ROOT}/tests/baseline"
if [[ -d "${baseline_root}" ]]; then
  while IFS= read -r f; do
    if is_native_vit "$f"; then
      vit_files+=("$f")
    fi
  done < <(find "${baseline_root}" -type f -name '*.vit' | sort)
fi

if [[ ${#vit_files[@]} -eq 0 ]]; then
  echo "No native .vit files found for selection"
  exit 0
fi

echo "Checking ${#vit_files[@]} native .vit files"
"${CHECKER}" "${vit_files[@]}"
