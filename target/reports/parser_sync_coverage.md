# Parser Sync Coverage

- Grammar: `src/vitte/grammar/vitte.ebnf`
- Parser corpus: `src/vitte/compiler/frontend/parse`
- Total rules: `271`
- Parsed evidence: `243`
- AST evidence: `206`
- Diagnostic evidence: `261`
- Test evidence: `250`
- Missing evidence: `0`

## Missing rules

- none

## Sample matrix

| Rule | Parsed | AST | Diag | Test |
| --- | --- | --- | --- | --- |
| `program` | yes | yes | yes | yes |
| `toplevel` | yes | no | yes | yes |
| `space_decl` | yes | yes | yes | yes |
| `use_decl` | yes | yes | yes | yes |
| `use_glob` | yes | yes | yes | yes |
| `use_group` | yes | yes | yes | yes |
| `import_items` | yes | yes | yes | yes |
| `import_item` | yes | yes | yes | yes |
| `export_decl` | yes | yes | yes | yes |
| `const_decl` | yes | yes | yes | yes |
| `static_decl` | yes | yes | yes | yes |
| `global_decl` | yes | yes | yes | yes |
| `type_alias_decl` | yes | yes | yes | yes |
| `opaque_type_decl` | yes | yes | yes | yes |
| `extern_type_decl` | yes | yes | yes | yes |
| `form_decl` | yes | yes | yes | yes |
| `class_decl` | yes | yes | yes | yes |
| `union_decl` | yes | yes | yes | yes |
| `form_items` | yes | yes | yes | yes |
| `form_item` | yes | yes | yes | yes |
| `class_items` | yes | yes | yes | yes |
| `class_item` | yes | yes | yes | yes |
| `field_list` | no | no | yes | yes |
| `field_item` | yes | yes | yes | yes |
| `bits_decl` | yes | yes | yes | yes |
| `bitfield_list` | yes | yes | yes | yes |
| `bitfield_item` | yes | yes | yes | yes |
| `pick_decl` | yes | yes | yes | yes |
| `case_list` | yes | no | yes | yes |
| `case_item` | yes | no | yes | yes |
| `case_payload` | yes | no | yes | yes |
| `case_field` | yes | yes | yes | yes |
| `flags_decl` | yes | yes | yes | yes |
| `flag_list` | no | no | no | yes |
| `flag_item` | no | no | no | yes |
| `trait_decl` | yes | yes | yes | yes |
| `trait_item` | yes | yes | yes | yes |
| `associated_type_decl` | yes | yes | yes | yes |
| `impl_decl` | yes | yes | yes | yes |
| `impl_head` | yes | yes | yes | yes |

