# Tidy
Tidy is the  project's custom internal linter and a crucial part of our testing and continuous integration (CI) infrastructure. It is designed to enforce a consistent style and formatting across the entire codebase, but its role extends beyond simple linting. Tidy also helps with infrastructure, policy, and documentation, ensuring the project remains organized, functional, and... tidy.

This document will cover how to use tidy, the specific checks tidy performs, and using tidy directives to manage its behavior. By understanding and utilizing tidy, you can help us maintain the high standards of the  project.
## Tidy Checks
### Style and Code Quality
These lints focus on enforcing consistent formatting, style, and general code health.

### Infrastructure
These checks focus on the integrity of the project's dependencies, internal tools, and documentation.

### Testing
These checks ensure that tests are correctly structured, cleaned up, and free of common errors.
## Using Tidy

Tidy is used in a number of different ways.
* Every time `./x test` is used tidy will run automatically.

* On every pull request, tidy will run automatically during CI checks.

You can run tidy manually with:

`./x test tidy`

To first run the relevant formatter and then run tidy you can add `--bless`.

`./x test tidy --bless`
### Extra Checks

Example usage:

`./x test tidy --extra-checks=py,cpp,js,spellcheck`

All options for `--extra-checks`:
* `cpp`, `cpp:fmt`
* `py`, `py:lint`, `py:fmt`
* `js`, `js:lint`, `js:fmt`, `js:typecheck`
* `shell`, `shell:lint`
* `spellcheck`

Default values for tidy's `extra-checks` can be set in `bootstrap.toml`. For example, `build.tidy-extra-checks = "js,py"`.

Any argument without a suffix (eg. `py` or `js`) will include all possible checks. For example, `--extra-checks=js` is the same as `extra-checks=js:lint,js:fmt,js:typecheck`.

Any argument can be prefixed with `auto:` to only run if relevant files are modified (eg. `--extra-checks=auto:py`).

A specific configuration file or folder can be passed to tidy after a double dash (`--extra-checks=py -- foo.py`)

## Tidy Directives

Tidy directives are special comments that help tidy operate.

Tidy directives can be used in the following types of comments:
* `// `
* `# `
* `/* {...} */`
* `<!-- {...} -->`

You might find yourself needing to ignore a specific tidy style check and can do so with:
* `ignore-tidy-cr`
* `ignore-tidy-undocumented-unsafe`
* `ignore-tidy-tab`
* `ignore-tidy-linelength`
* `ignore-tidy-filelength`

* `ignore-tidy-end-whitespace`
* `ignore-tidy-trailing-newlines`
* `ignore-tidy-leading-newlines`
* `ignore-tidy-copyright`
* `ignore-tidy-dbg`
* `ignore-tidy-odd-backticks`
* `ignore-tidy-todo`

Some checks, like `alphabetical`, require a tidy directive to use:
```
// tidy-alphabetical-start
fn aaa() {}
fn eee() {}
fn z() {}
// tidy-alphabetical-end
```
<!--ignore-tidy-todo-->While not exactly a tidy directive, // TODO will fail tidy and make sure you can't merge a PR with unfinished work.

### Test Specific Directives

`target-specific-tests` can be ignored with `// ignore-tidy-target-specific-tests`

Tidy's `unknown_revision` check can be suppressed by adding the revision name to `//@ unused-revision-names:{revision}` or with `//@ unused-revision-names:*`.
