# CMake generated Testfile for 
# Source directory: /Users/vincent/Documents/Github/vitte
# Build directory: /Users/vincent/Documents/Github/vitte/build-rust
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[test_vitte_lexer]=] "/Users/vincent/Documents/Github/vitte/build-rust/test_vitte_lexer")
set_tests_properties([=[test_vitte_lexer]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/CMakeLists.txt;47;add_test;/Users/vincent/Documents/Github/vitte/CMakeLists.txt;0;")
add_test([=[test_vitte_parser_phrase]=] "/Users/vincent/Documents/Github/vitte/build-rust/test_vitte_parser_phrase")
set_tests_properties([=[test_vitte_parser_phrase]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/CMakeLists.txt;51;add_test;/Users/vincent/Documents/Github/vitte/CMakeLists.txt;0;")
add_test([=[test_vitte_desugar_phrase]=] "/Users/vincent/Documents/Github/vitte/build-rust/test_vitte_desugar_phrase")
set_tests_properties([=[test_vitte_desugar_phrase]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/CMakeLists.txt;55;add_test;/Users/vincent/Documents/Github/vitte/CMakeLists.txt;0;")
add_test([=[test_vitte_codegen]=] "/Users/vincent/Documents/Github/vitte/build-rust/test_vitte_codegen")
set_tests_properties([=[test_vitte_codegen]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/CMakeLists.txt;59;add_test;/Users/vincent/Documents/Github/vitte/CMakeLists.txt;0;")
add_test([=[test_vitte_diag_golden]=] "/Users/vincent/Documents/Github/vitte/build-rust/test_vitte_diag_golden")
set_tests_properties([=[test_vitte_diag_golden]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/CMakeLists.txt;64;add_test;/Users/vincent/Documents/Github/vitte/CMakeLists.txt;0;")
subdirs("bench")
subdirs("compiler")
