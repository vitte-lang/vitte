# lingua/syntax/vitte_attr_parsing/messages.ftl
# Diagnostic messages for attribute parsing (token + AST adapters).
# Language: English
#
# Conventions:
# - Message IDs are kebab-case with prefix "attr-".
# - Use { $var } placeables for dynamic content.
# - Keep primary messages short; put actionable guidance in *.help.

attr-stage-name =
    Attribute Parsing


### Core expectations

attr-expected-at =
    Expected '@' to start an attribute.

attr-expected-name =
    Expected an attribute name after '@'.
attr-expected-name.help =
    Example: @inline, @cfg(os="linux"), @deprecated(note="...")

attr-expected-lparen =
    Expected '(' after attribute name.

attr-expected-rparen =
    Expected ')' to close attribute arguments.
attr-expected-rparen.help =
    Ensure parentheses are balanced; remove stray commas before ')'.

attr-expected-rbrack =
    Expected ']' to close list literal in attribute arguments.

attr-expected-rbrace =
    Expected '}' to close map literal in attribute arguments.

attr-expected-colon =
    Expected ':' in map literal entry (key: value).
attr-expected-colon.help =
    Map entries must be written as { key: value, ... }.

attr-expected-eq =
    Expected '=' for named argument assignment (key = value).

attr-expected-comma-or-close =
    Expected ',' or closing delimiter.

attr-expected-value =
    Expected a value in attribute arguments.
attr-expected-value.help =
    Supported values: null, true/false, int, float, "text", name/path, [list], {map}.


### Names / paths

attr-expected-name-after-slash =
    Expected a name after '/' in a path value.
attr-expected-name-after-slash.help =
    Use clean path segments, e.g. os/linux, arch/x86_64.

attr-expected-name-after-dot =
    Expected a name after '.' in a dotted name value.

attr-expected-name-after-dcolon =
    Expected a name after '::' in a qualified name value.

attr-name-invalid =
    Attribute name is invalid: { $name }.
attr-name-invalid.help =
    Attribute names should be identifiers; avoid spaces and punctuation.

attr-name-empty =
    Attribute name is empty.

attr-path-invalid =
    Path value is invalid: { $path }.
attr-path-invalid.help =
    Use non-empty segments separated by '/', without trailing '/'.

attr-map-key-invalid =
    Map key is invalid: { $key }.
attr-map-key-invalid.help =
    Map keys must be a name or a string literal.

attr-expected-map-key =
    Expected a map key (name or "text") inside '{ }'.


### Values

attr-int-invalid =
    Invalid integer literal: { $lex }.

attr-float-invalid =
    Invalid float literal: { $lex }.

attr-text-invalid =
    Invalid string literal.

attr-bool-invalid =
    Invalid boolean literal: { $lex }.

attr-null-invalid =
    Invalid null literal: { $lex }.

attr-list-item-invalid =
    Invalid list item in attribute arguments.

attr-map-entry-invalid =
    Invalid map entry in attribute arguments.
attr-map-entry-invalid.help =
    Each entry must be key: value, separated by commas.

attr-trailing-comma =
    Trailing comma in attribute arguments.


### Argument rules

attr-arg-duplicate-named =
    Duplicate named argument: { $key }.
attr-arg-duplicate-named.help =
    Remove the duplicate or merge the arguments. If dedup is enabled, the last value may win.

attr-arg-mixed-forms =
    Mixed argument forms detected.
attr-arg-mixed-forms.help =
    Mixing positional and named arguments is allowed, but keep it intentional and consistent.

attr-arg-empty =
    Attribute argument list is empty.

attr-arg-too-many =
    Too many arguments for attribute { $name }.
attr-arg-too-many.help =
    Verify the attribute schema; remove unused arguments.

attr-arg-missing-required =
    Missing required argument { $key } for attribute { $name }.

attr-arg-unknown-key =
    Unknown named argument { $key } for attribute { $name }.
attr-arg-unknown-key.help =
    Check spelling or update the registry/schema for this attribute.

attr-arg-type-mismatch =
    Attribute { $name } argument { $key } has wrong type: expected { $expected }, got { $got }.


### Normalization

attr-normalize-trimmed =
    Trimmed whitespace in attribute { $name }.

attr-normalize-sorted-named =
    Sorted named arguments for attribute { $name }.

attr-normalize-dedup-named =
    Deduplicated named arguments for attribute { $name }.

attr-normalize-sorted-map =
    Sorted map keys in attribute { $name } value.

attr-normalize-dedup-map =
    Deduplicated map keys in attribute { $name } value.


### cfg attribute helpers

attr-cfg-invalid =
    Invalid cfg expression in @cfg(...).
attr-cfg-invalid.help =
    Supported:
      @cfg(os="linux")
      @cfg(arch="x86_64")
      @cfg(feature="foo")
      @cfg(any(os="linux", os="macos"))
      @cfg(all(feature="a", feature="b"))
      @cfg(not(feature="test"))

attr-cfg-unknown-op =
    Unknown cfg operator: { $op }.
attr-cfg-unknown-op.help =
    Use any, all, or not.

attr-cfg-not-arity =
    cfg not(...) expects exactly one inner condition.

attr-cfg-empty =
    cfg expression is empty.

attr-cfg-unknown-key =
    Unknown cfg key: { $key }.
attr-cfg-unknown-key.help =
    Common keys: os, arch, feature.

attr-cfg-value-invalid =
    cfg value is invalid for key { $key }.
attr-cfg-value-invalid.help =
    Use a string literal, e.g. os="linux".

attr-cfg-feature-empty =
    cfg feature name is empty.

attr-cfg-os-empty =
    cfg os name is empty.

attr-cfg-arch-empty =
    cfg arch name is empty.


### Recovery / internal

attr-recovery-skip-token =
    Skipping unexpected token during attribute parsing.

attr-recovery-insert-null =
    Inserted null value for recovery.

attr-recovery-continue =
    Continuing after attribute parsing error.

attr-internal-error =
    Internal attribute parsing error: { $detail }.

attr-summary =
    Parsed { $attrs } attribute(s): { $values } value(s), { $pairs } pair(s), { $recovered } recovery step(s).
