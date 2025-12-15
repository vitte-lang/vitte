#!/usr/bin/env sh
set -eu

# Créer la structure des dossiers
mkdir -p src/compiler src/runtime src/stdlib src/debug
mkdir -p build/{obj,bin,lib,coverage}
mkdir -p tests/{unit,integration,fixtures,performance}
mkdir -p docs/{guide,api,examples,architecture,internals}
mkdir -p tools/{scripts,analyzers}
mkdir -p include/{compiler,runtime,stdlib,debug}
mkdir -p cmake/modules
mkdir -p benchmarks/{data,results}

# Compiler
touch src/compiler/{lexer.c,parser.c,codegen.c,ast.c,token.c,error.c,optimizer.c,linker.c,preprocessor.c,symboltable.c}
touch include/compiler/{lexer.h,parser.h,codegen.h,ast.h,token.h,error.h,optimizer.h,linker.h,symboltable.h}

# Runtime
touch src/runtime/{vm.c,memory.c,builtins.c,gc.c,value.c,stack.c,interpreter.c,executor.c,loader.c,context.c}
touch include/runtime/{vm.h,memory.h,gc.h,value.h,stack.h,interpreter.h,executor.h,loader.h,context.h}

# Stdlib
touch src/stdlib/{io.c,math.c,string.c,array.c,hashmap.c,file.c,time.c,json.c,regex.c,threading.c}
touch include/stdlib/{io.h,math.h,string.h,array.h,hashmap.h,file.h,time.h,json.h,regex.h,threading.h}

# Debug
touch src/debug/{debugger.c,profiler.c,tracer.c,breakpoint.c}
touch include/debug/{debugger.h,profiler.h,tracer.h,breakpoint.h}

# Tests
touch tests/unit/{test_lexer.c,test_parser.c,test_vm.c,test_memory.c,test_gc.c,test_stdlib.c,test_main.c}
touch tests/integration/{test_integration.c,test_end_to_end.c,run_tests.sh}
touch tests/performance/{bench_vm.c,bench_gc.c,bench_parser.c}
touch tests/fixtures/{example1.steel,example2.steel,complex.steel,stress.steel}

# Documentation
touch docs/{README.md,CONTRIBUTING.md,CHANGELOG.md,LICENSE}
touch docs/guide/{getting-started.md,syntax.md,stdlib.md,advanced.md,faq.md}
touch docs/api/{compiler.md,runtime.md,vm.md,stdlib-api.md}
touch docs/architecture/{overview.md,design.md,bytecode.md,memory-model.md}
touch docs/internals/{optimization.md,gc-algorithm.md,compilation-stages.md}
touch docs/examples/{hello.steel,fibonacci.steel,factorial.steel,oop.steel,generics.steel}

# Tools
touch tools/scripts/{format.sh,lint.sh,debug.sh,profile.sh,test.sh,coverage.sh,release.sh}
touch tools/analyzers/{static-check.c,security-check.c}

# Config files
touch .editorconfig .clang-format .gitignore CMakeLists.txt
touch build/CMakeLists.txt
touch Makefile config.mk VERSION

echo "✓ Bootstrap complété: architecture maximale initialisée"
