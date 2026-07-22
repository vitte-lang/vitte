# Package Workspace Example

This example is a real multi-package layout used by the package manager gate.

Commands:

```sh
vitte package check --path packages/app
vitte package build --path packages/app
vitte package graph
vitte package graph explain
vitte package publish --dry-run --path packages/app
vitte workspace build --package workspace-app
vitte workspace test --all
```
