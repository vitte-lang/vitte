# CMake generated Testfile for 
# Source directory: /Users/vincent/Documents/Github/vitte/compiler
# Build directory: /Users/vincent/Documents/Github/vitte/build-norust/compiler
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_lexer "/Users/vincent/Documents/Github/vitte/build-norust/compiler/test_lexer")
set_tests_properties(test_lexer PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;126;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
add_test(test_ast "/Users/vincent/Documents/Github/vitte/build-norust/compiler/test_ast")
set_tests_properties(test_ast PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;132;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
add_test(test_types "/Users/vincent/Documents/Github/vitte/build-norust/compiler/test_types")
set_tests_properties(test_types PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;138;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
add_test(test_symbol_table "/Users/vincent/Documents/Github/vitte/build-norust/compiler/test_symbol_table")
set_tests_properties(test_symbol_table PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;144;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
add_test(test_diag "/Users/vincent/Documents/Github/vitte/build-norust/compiler/test_diag")
set_tests_properties(test_diag PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;150;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
add_test(integration_cli_tokens "/Users/vincent/Documents/Github/vitte/compiler/../tests/integration/cli_tokens/run_cli_tokens.sh" "/Users/vincent/Documents/Github/vitte/build-norust/compiler/vittec" "/Users/vincent/Documents/Github/vitte/compiler/../tests/integration/cli_tokens/sample_program.vitte" "/Users/vincent/Documents/Github/vitte/compiler/../tests/integration/cli_tokens/expected_tokens.txt")
set_tests_properties(integration_cli_tokens PROPERTIES  _BACKTRACE_TRIPLES "/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;194;add_test;/Users/vincent/Documents/Github/vitte/compiler/CMakeLists.txt;0;")
