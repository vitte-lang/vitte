# Extension tests and capability maintenance

This guide describes the checks required before advertising an editor feature or changing a Vitte extension workflow.

## Run the checks

From this extension directory:

```sh
npm ci
npm run check:imports-sync
npm run check:manifest
npm run test:unit
npm test
npm run vsix
npm run vsix:verify-artifact
npm run vsix:smoke
```

`test:unit` builds the extension and runs the portable Node suites, including language-server protocol, compiler diagnostic, project, process-safety and regression-fixture tests. `npm test` also launches a VS Code extension host for activation, commands, language-server integration, refactorings and diagnostics. The host downloads the configured VS Code version unless `VSCODE_EXECUTABLE_PATH` points to an installed editor executable.

Native DWARF runtime scenarios are experimental and are skipped by default. To run them, use `VITTE_RUN_NATIVE_DAP_TESTS=1 npm test` on Linux x86_64 / Ubuntu 24.04 with the audited Vitte compiler, LLDB and Expect installed. The adapter configuration tests remain part of the normal cross-platform suite.

The VSIX smoke installs the artifact into an isolated profile and temporary workspace, then checks install, update, rollback, activation, editor-host workflows, shutdown and uninstall. The editor-host regression subset loads the installed `VitteStudio.vitte-studio` package while test code comes from `out/test`; it does not load the development extension. CI runs packaging and activation on Linux x64, macOS arm64 and Windows x64, with a separate Linux run at the declared VS Code minimum, 1.105.0. Advertised capability coverage and the VSIX smoke must pass before a release is ready.

## Add a language-server capability

1. Add the provider to `server/src/` and register it in `server/src/server.ts`. Prefer the shared lexer, indexer, project-scope and navigation APIs to editor-specific parsing. Pass request cancellation through long-running work and reject results whose document, configuration or workspace revision has changed.
2. Add positive and negative provider cases to `test/language.test.cjs`. For request/response behavior, exercise the stdio JSON-RPC server. Include incomplete syntax, unsaved documents, imports, nested scopes, multiple roots and Unicode offsets when relevant.
3. Add or extend fixtures under `test/fixtures/editor/`. Keep `test/fixtures/editor/capability-coverage.json` synchronized with every advertised row in `server/src/capability-matrix.json`; the unit suite fails when an advertised feature has no mapped positive and negative test, or a source-mutating feature lacks safe and unsafe fixtures.
4. Record the supported scope, limitations, provider and test evidence in the versioned capability matrix. Bump the matrix version and update the generated/reference capability page when matrix rows or claims change. Keep planned and unavailable features unadvertised.
5. Run the unit, extension-host and packaged-VSIX checks above. Do not use development-host results as evidence that the packaged VSIX works.

For source-changing behavior, test both the returned edits and the exact text they affect. Rename fixtures cover comments, strings, aliases and shadowed bindings; code-action fixtures distinguish a machine-applicable edit from syntax diagnostics that must not receive an invented fix. Add a refusal test for ambiguity or stale buffers whenever the operation cannot prove the edit safe.

## Add a command or setting

### Command

1. Contribute its identifier, title and arguments in `package.json`.
2. Register it in the appropriate extension module and add a command test in `src/test/suite/extension.test.ts`. The contributed-command test verifies every `vitte.*` manifest command was actually registered. Test malformed arguments, unavailable toolchains and user-visible failure output where applicable.
3. If the command starts a process, use the shared managed-process/task helpers, pass arguments as an array with shell execution disabled, honor workspace trust, and test termination and recovery.
4. Document its supported toolchain and project behavior in the README or getting-started guide. Update the capability matrix if it changes what the extension advertises.

### Setting

1. Define the setting in `package.json` with a JSON schema type, default, description, validation constraints and the narrowest correct VS Code scope (`resource`, `window`, or another documented scope).
2. Read it through the scoped `WorkspaceConfiguration` for the active workspace folder. Validate paths and values before use; do not silently fall back to a different compiler or project configuration.
3. Add a manifest/behavior assertion in `test/language.test.cjs`, `test/project.test.cjs` or the extension-host suite. For folder-scoped settings, cover at least two workspace roots and prove values do not leak between them.
4. Document precedence and limitations in the setting description plus the user guide.

## Fixtures and platform evidence

Use small source fixtures with readable names. Keep a valid baseline and an invalid/incomplete baseline, then add focused files for imports, multiple modules, edits, Unicode or other feature-specific cases. Test UTF-8 compiler byte spans against LSP UTF-16 ranges around multibyte characters. Tests that change source must prove unrelated comments, strings and shadowed symbols remain unchanged.

The platform matrix currently claims extension packaging/activation support on Linux x64, macOS arm64 and Windows x64. Compiler, formatter and debugger support are separate capability-matrix claims and must not be inferred from extension activation. Keep platform-specific paths, executable permissions, environment and process-shutdown checks in the platform suites. Use an installed VS Code profile and a VSIX outside the checkout for release packaging evidence.

## Screenshots and short demos

When a user-visible workflow changes, update its screenshot or short demonstration in `docs/media/extension/` in the same change. The README and getting-started walkthrough should point to current assets when they materially help the workflow. Keep captures focused on the installed extension, show its relevant output/status, and remove user paths, source data, credentials and unrelated desktop content. If the UI did not change, no new capture is needed.
