# Borrowck Fixtures

| Fixture | Status |
| --- | --- |
| src/vitte/compiler/tests/diagnostics/borrowck/complex_alias_path.json.snap | present |
| src/vitte/compiler/tests/diagnostics/borrowck/complex_alias_path.txt.snap | present |
| src/vitte/compiler/tests/diagnostics/borrowck/complex_alias_path.vit | present |
| src/vitte/compiler/tests/diagnostics/borrowck/use_after_move.json.snap | present |
| src/vitte/compiler/tests/diagnostics/borrowck/use_after_move.txt.snap | present |
| src/vitte/compiler/tests/diagnostics/borrowck/use_after_move.vit | present |
| tests/borrowck/invalid/move_after_move.vit | move after move |
| tests/borrowck/invalid/mutable_borrow_conflict.vit | mutable borrow conflict |
| tests/borrowck/invalid/partial_move_root_use.vit | partial move then root use |
| tests/borrowck/invalid/return_ref_to_local.vit | return reference to local |
| tests/borrowck/invalid/use_after_move.vit | use after move |
| tests/borrowck/invalid/write_while_borrowed.vit | write while borrowed |
| tests/borrowck/multifile/app.vit | multifile app using exported non-Copy type and taking ownership |
| tests/borrowck/multifile/box.vit | multifile exported non-Copy type |
| tests/borrowck/multifile/take.vit | multifile imported function taking ownership |
| tests/borrowck/valid/basic.vit | valid basic ownership |
| tests/borrowck/valid/borrow_scope_end.vit | borrow expires at scope end |
| tests/borrowck/valid/copy_does_not_move.vit | copy type does not move source |
| tests/borrowck/valid/reinit_after_move.vit | valid reinitialization after move |
