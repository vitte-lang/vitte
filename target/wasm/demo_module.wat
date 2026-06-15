(module
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (func (export "main") (result i32)
    i32.const 0)
)
