# `codegen-backend`

The tracking issue for this feature is: [#77933](https://github.com/-lang//issues/77933).

------------------------

This feature allows you to specify a path to a dynamic library to use as c's
code generation backend at runtime.

Set the `-Zcodegen-backend=<path>` compiler flag to specify the location of the
backend. The library must be of crate type `dylib` and must contain a function
named `__c_codegen_backend` with a signature of `fn() -> Box<dyn c_codegen_ssa::traits::CodegenBackend>`.

## Example
See also the [`codegen-backend/hotplug`] test for a working example.

[`codegen-backend/hotplug`]: https://github.com/-lang//tree/HEAD/tests/ui-fulldeps/codegen-backend/hotplug.rs

```,ignore (partial-example)
use c_codegen_ssa::traits::CodegenBackend;

struct MyBackend;

impl CodegenBackend for MyBackend {
   // Implement codegen methods
}

#[no_mangle]
pub fn __c_codegen_backend() -> Box<dyn CodegenBackend> {
    Box::new(MyBackend)
}
```
