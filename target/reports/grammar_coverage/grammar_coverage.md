# Grammar Coverage Report

- Grammar: `src/vitte/grammar/vitte.ebnf`
- Rule count: `256`
- Classified rules: `256`
- Parsed: `226`
- AST built: `210`
- Diagnosed: `246`
- Tested: `246`
- Missing: `0`
- Overall status: `green`

## Coverage Matrix

| Rule | Parsed | AST | Diag | Test | Missing | Classification |
| --- | --- | --- | --- | --- | --- | --- |
| `program` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `toplevel` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `space_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `use_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `use_glob` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `use_group` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `import_items` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `import_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `export_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `const_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `static_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `region_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `region_kind` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `region_capabilities` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_alias_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `opaque_type_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_type_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `form_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `class_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `union_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `form_items` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `form_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `class_items` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `class_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `field_list` | no | no | yes | yes | no | diagnosed, tested |
| `field_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bits_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bitfield_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bitfield_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pick_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `case_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `case_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `case_payload` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `case_field` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `flags_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `flag_list` | no | no | no | yes | no | tested |
| `flag_item` | no | no | no | yes | no | tested |
| `trait_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `trait_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `associated_type_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `impl_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `impl_head` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `impl_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_signature` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_name` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `operator_symbol` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_modifier` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_block` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_proc_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_clause` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `extern_abi` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `callconv` | yes | yes | yes | no | no | parsed, ast-built, diagnosed |
| `intrinsic_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `macro_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `macro_param_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `macro_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `macro_kind` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `static_assert_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `test_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bench_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `entry_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `visibility` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `visibility_scope` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `generic_params` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `generic_param_list` | yes | yes | yes | no | no | parsed, ast-built, diagnosed |
| `generic_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `where_clause` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `where_bounds` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `where_bound` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `trait_bound` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `param_list` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `param` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `self_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `normal_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `variadic_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `param_mode` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `effects_clause` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `requires_clause` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `capability_list` | no | no | no | yes | no | tested |
| `capability` | no | no | no | yes | no | tested |
| `module_path` | no | no | yes | yes | no | diagnosed, tested |
| `package_path` | no | no | yes | no | no | diagnosed |
| `relative` | no | no | yes | no | no | diagnosed |
| `package_parts` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `ident_list` | no | no | no | yes | no | tested |
| `query_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `query_body` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `compiler_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pass_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `backend_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `diagnostic_decl` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `ident_or_string` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `meta_args` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `meta_arg_list` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `meta_arg` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `meta_tail` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `attr_prefix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `inner_attribute` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `outer_attribute` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `doc_comment` | no | no | yes | yes | no | diagnosed, tested |
| `docstring` | no | no | yes | no | no | diagnosed |
| `attr_path` | no | yes | yes | no | no | ast-built, diagnosed |
| `attr_arg_list` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `attr_arg` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `block` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `stmt_end` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `local_const_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `let_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `set_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `assign_target` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `give_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `try_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `defer_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `asm_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `asm_arg_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `asm_arg` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `unsafe_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `emit_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `assert_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `panic_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `unreachable_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `if_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `while_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `loop_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `for_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `break_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `continue_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `select_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `match_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `match_arm_body` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `when_match_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `with_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `critical_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `expr_stmt` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `assign_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `assign_op` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `ternary_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `coalesce_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `range_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `or_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `and_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bit_or_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bit_xor_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bit_and_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `eq_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `rel_expr` | no | no | no | yes | no | tested |
| `rel_op` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `shift_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `add_expr` | no | no | yes | yes | no | diagnosed, tested |
| `mul_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `cast_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `unary_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `postfix_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `call_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `member_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `index_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `try_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `await_suffix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `index_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `slice_expr` | yes | yes | yes | no | no | parsed, ast-built, diagnosed |
| `primary` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `path_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `arg_list` | no | no | no | yes | no | tested |
| `arg` | no | no | no | yes | no | tested |
| `tuple_lit` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `set_lit` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `map_lit` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `map_items` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `map_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bytes_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `resource_lit` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `resource_items` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `resource_item` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `struct_lit` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_path` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `field_init_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `field_init` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `if_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `match_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `lambda_expr` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `unsafe_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `builtin_expr` | no | no | yes | yes | no | diagnosed, tested |
| `sizeof_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `alignof_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `offsetof_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `typeof_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `nameof_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_or` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_atom` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_bind` | no | no | no | yes | no | tested |
| `pattern_ctor` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_head` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_args` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_struct` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_fields` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_field` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_tuple` | no | yes | yes | yes | no | ast-built, diagnosed, tested |
| `pattern_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pattern_range` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_expr` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_union` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_prefix` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_qualifier` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_qual` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `addr_space` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_atom` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `reference_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `pointer_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `optional_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `fixed_array_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `slice_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `tuple_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `proc_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_list` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_param` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `dyn_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `impl_trait_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `type_primary` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `lifetime` | no | no | yes | no | no | diagnosed |
| `primitive_type` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `literal` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `list_lit` | no | no | yes | yes | no | diagnosed, tested |
| `list_comp` | yes | yes | yes | yes | no | parsed, ast-built, diagnosed, tested |
| `bool_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `null_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `int_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `float_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `exponent` | no | no | yes | no | no | diagnosed |
| `char_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `char_char` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `string_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `string_char` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `escape_seq` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `raw_string_lit` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `raw_string_char` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `line_comment` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `block_comment` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `zone_comment` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `ident` | no | no | no | yes | no | tested |
| `suffix` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `WS` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `WS1` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `NEWLINE` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `LETTER` | yes | no | yes | no | no | parsed, diagnosed |
| `DIGIT` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `BINDIGIT` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `OCTDIGIT` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `HEXDIGIT` | yes | no | yes | yes | no | parsed, diagnosed, tested |
| `EOF` | yes | no | yes | yes | no | parsed, diagnosed, tested |

## Missing Rules

- none

