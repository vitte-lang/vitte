# lingua/syntax/vitte_ast_lowering/messages.ftl
# Diagnostic messages for the AST -> AST-IR lowering stage.
# Language: English
#
# Conventions:
# - Message IDs are kebab-case with prefix "lowering-".
# - Use { $var } placeables for dynamic content.
# - Keep primary messages short; put actionable guidance in *.help where useful.

lowering-stage-name =
    AST Lowering

lowering-note-adapter-mode =
    Using adapter-based lowering (generic AST nodes).

lowering-note-typed-mode =
    Using typed AST lowering (project-provided nodes).


### File / top-level

lowering-unexpected-non-item-toplevel =
    Unexpected node at file top-level: expected an item, got { $kind }.
lowering-unexpected-non-item-toplevel.help =
    Ensure the parser produces only top-level items under the file node (space/pull/share/build/form/pick/bond/proc/flow/entry/global).

lowering-file-missing-items =
    File node contains no items.
lowering-file-missing-items.help =
    If this is intentional (empty module), suppress this warning; otherwise verify the parser output.

lowering-file-attrs-invalid =
    Invalid attribute(s) on file node.
lowering-file-attrs-invalid.help =
    Check attribute spelling and allowed targets (file-level attributes only).

lowering-file-attrs-normalized =
    File attributes were normalized.


### Item dispatch

lowering-unknown-item-tag =
    Unknown item tag: { $tag }.
lowering-unknown-item-tag.help =
    Expected one of: space, pull, share, build, form, pick, bond, proc, flow, entry, global.

lowering-item-missing-tag =
    Item node is missing a tag.
lowering-item-missing-tag.help =
    The parser must set a stable tag identifying the item kind.

lowering-item-missing-name =
    Item is missing a name.
lowering-item-missing-name.help =
    Provide a name in node.text for named items (form/pick/bond/proc/flow/global).

lowering-item-attrs-invalid =
    Invalid attribute(s) on item { $item }.
lowering-item-attrs-invalid.help =
    Attributes must be valid for the specific item target (e.g. Proc/Form/Bond).

lowering-item-attrs-normalized =
    Attributes on item { $item } were normalized.

lowering-item-duplicate =
    Duplicate item name in the same scope: { $name }.
lowering-item-duplicate.help =
    Rename one of the items or move it into a different space/module.

lowering-item-reserved-name =
    Reserved item name: { $name }.
lowering-item-reserved-name.help =
    Pick a different name; reserved words are not allowed as item identifiers.


### space

lowering-space-missing-path =
    space item is missing its path.
lowering-space-missing-path.help =
    Provide the module path in node.text, e.g. "space a/b/c".

lowering-space-path-invalid =
    space path is invalid: { $path }.
lowering-space-path-invalid.help =
    Use a clean path with segments separated by '/', without empty segments.

lowering-space-multiple =
    Multiple space declarations in the same file.
lowering-space-multiple.help =
    Keep only one space declaration per file (or ensure later declarations are explicitly allowed by the language rules).


### pull

lowering-pull-missing-path =
    pull item is missing its path.
lowering-pull-missing-path.help =
    Provide the pulled module path in node.text, e.g. "pull std/text as text".

lowering-pull-path-invalid =
    pull path is invalid: { $path }.

lowering-pull-as-missing-name =
    pull alias is missing a name.
lowering-pull-as-missing-name.help =
    Provide the alias identifier in the child node tagged "as".

lowering-pull-only-empty =
    pull only-list is empty.
lowering-pull-only-empty.help =
    Either remove the "only" clause or specify at least one imported name.

lowering-pull-only-invalid-name =
    pull only-list contains an invalid name: { $name }.

lowering-pull-duplicate-alias =
    pull alias duplicates an existing name in scope: { $name }.
lowering-pull-duplicate-alias.help =
    Choose a different alias to avoid shadowing.


### share

lowering-share-empty =
    share item exports nothing.
lowering-share-empty.help =
    Use "share all" or provide at least one name to share.

lowering-share-name-invalid =
    share item contains an invalid name: { $name }.

lowering-share-duplicate-name =
    share item contains a duplicate name: { $name }.

lowering-share-all-with-names =
    share all cannot be combined with an explicit name list.
lowering-share-all-with-names.help =
    Use either "share all" or "share name1, name2, ...", not both.


### build

lowering-build-missing-key =
    build item is missing its key.
lowering-build-missing-key.help =
    Provide the key in node.text, e.g. "build target = ...".

lowering-build-missing-value =
    build item is missing its value.
lowering-build-missing-value.help =
    Provide a literal value node as the first child (string, int, bool, ...).

lowering-build-non-literal-value =
    build item value must be a literal, got { $kind }.
lowering-build-non-literal-value.help =
    Use a literal value (text/int/float/bool/null) in build declarations.

lowering-build-unknown-key =
    build key is not recognized: { $key }.
lowering-build-unknown-key.help =
    Verify the key name or register it as a supported build directive.


### form (struct-like)

lowering-form-missing-name =
    form item is missing its name.
lowering-form-missing-name.help =
    Provide the form name in node.text.

lowering-form-duplicate-field =
    Duplicate field name in form { $form }: { $field }.
lowering-form-duplicate-field.help =
    Each field must have a unique name.

lowering-form-field-missing-type =
    Field { $field } in form { $form } is missing its type.
lowering-form-field-missing-type.help =
    Provide a type node as a child of the field node.

lowering-form-field-invalid-init =
    Field { $field } initializer is invalid.
lowering-form-field-invalid-init.help =
    Field initializers must be valid expressions.

lowering-form-type-params-duplicate =
    Duplicate type parameter in form { $form }: { $name }.

lowering-form-type-params-empty =
    form type parameter list is empty.

lowering-form-visibility-invalid =
    form visibility is invalid: { $vis }.
lowering-form-visibility-invalid.help =
    Use "pub" or omit for hidden visibility.

lowering-form-field-attrs-invalid =
    Invalid attribute(s) on field { $field } in form { $form }.


### pick (enum-like)

lowering-pick-missing-name =
    pick item is missing its name.

lowering-pick-empty =
    pick { $pick } contains no cases.
lowering-pick-empty.help =
    Add at least one "case" to the pick.

lowering-pick-duplicate-case =
    Duplicate case name in pick { $pick }: { $case }.

lowering-pick-case-invalid =
    Invalid case declaration in pick { $pick }.
lowering-pick-case-invalid.help =
    A case must provide a case name and optional payload types.

lowering-pick-case-payload-invalid =
    Case { $case } payload type is invalid.

lowering-pick-type-params-duplicate =
    Duplicate type parameter in pick { $pick }: { $name }.


### bond (type alias / builtin bind)

lowering-bond-missing-name =
    bond item is missing its name.

lowering-bond-missing-target =
    bond { $bond } is missing its target type.
lowering-bond-missing-target.help =
    Provide a type node child describing the bound target (e.g. builtin_f32).

lowering-bond-target-invalid =
    bond { $bond } target type is invalid.

lowering-bond-duplicate =
    Duplicate bond name: { $name }.


### proc / flow

lowering-proc-missing-name =
    proc item is missing its name.

lowering-flow-missing-name =
    flow item is missing its name.

lowering-proc-missing-body =
    proc { $name } is missing its body block.
lowering-proc-missing-body.help =
    Ensure the parser produces a child node tagged "block" for proc bodies.

lowering-flow-missing-body =
    flow { $name } is missing its body block.

lowering-proc-params-invalid =
    proc { $name } parameters are invalid.

lowering-flow-params-invalid =
    flow { $name } parameters are invalid.

lowering-param-missing-name =
    Parameter is missing its name.

lowering-param-type-invalid =
    Parameter { $name } type is invalid.

lowering-param-default-invalid =
    Parameter { $name } default value is invalid.

lowering-return-type-invalid =
    Return type is invalid.

lowering-proc-visibility-invalid =
    proc visibility is invalid: { $vis }.

lowering-flow-visibility-invalid =
    flow visibility is invalid: { $vis }.


### entry

lowering-entry-kind-unknown =
    entry kind is unknown: { $kind }.
lowering-entry-kind-unknown.help =
    Supported entry kinds: app, service, tool, pipeline, driver, kernel.

lowering-entry-missing-path =
    entry is missing its target path.
lowering-entry-missing-path.help =
    Provide the target path in the first child node (node.text), e.g. "entry tool a/b".

lowering-entry-missing-body =
    entry is missing its body block.

lowering-entry-path-invalid =
    entry path is invalid: { $path }.


### global

lowering-global-missing-name =
    global item is missing its name.

lowering-global-missing-init =
    global { $name } is missing an initializer.
lowering-global-missing-init.help =
    Provide an expression node for the initializer.

lowering-global-type-invalid =
    global { $name } type is invalid.

lowering-global-init-invalid =
    global { $name } initializer is invalid.

lowering-global-visibility-invalid =
    global visibility is invalid: { $vis }.


### Type lowering

lowering-type-unknown-tag =
    Unknown type node tag: { $tag }.
lowering-type-unknown-tag.help =
    Expected type tags like builtin, named, list_of, map_of, pack_of, or.

lowering-type-missing-text =
    Type node is missing its textual payload.

lowering-type-named-path-invalid =
    Named type path is invalid: { $path }.

lowering-type-args-invalid =
    Type argument list is invalid.

lowering-type-map-arity =
    map_of requires exactly two type children (key, value).

lowering-type-or-arity =
    or requires exactly two type children (left, right).


### Statement lowering

lowering-unknown-stmt-tag =
    Unknown statement tag: { $tag }.
lowering-unknown-stmt-tag.help =
    Ensure statement tags match the surface grammar (make/keep/set/if/loop_while/loop_until/each/select/give/emit/defer/assert/halt/next).

lowering-make-missing-name =
    make statement is missing its name.
lowering-make-missing-name.help =
    Provide the local name in node.text.

lowering-make-missing-init =
    make { $name } is missing its initializer.
lowering-make-missing-init.help =
    Provide an expression node as initializer or omit if allowed.

lowering-make-type-invalid =
    make { $name } type is invalid.

lowering-keep-missing-name =
    keep statement is missing its name.

lowering-keep-missing-init =
    keep { $name } is missing its initializer.

lowering-keep-type-invalid =
    keep { $name } type is invalid.

lowering-set-missing-target =
    set statement is missing its target.

lowering-set-missing-value =
    set statement is missing its value.

lowering-set-op-invalid =
    set operator is invalid: { $op }.
lowering-set-op-invalid.help =
    Supported operators: =, +=, -=, *=, /=, %=.

lowering-if-missing-cond =
    if statement is missing its condition.

lowering-if-missing-then =
    if statement is missing its then-block.

lowering-elif-invalid =
    elif clause is invalid.
lowering-elif-invalid.help =
    An elif must contain (condition, block).

lowering-else-invalid =
    else clause is invalid.
lowering-else-invalid.help =
    An else must contain exactly one block.

lowering-loop-missing-cond =
    loop is missing its condition.

lowering-loop-missing-body =
    loop is missing its body block.

lowering-each-missing-item =
    each is missing its item binding name.

lowering-each-missing-iterable =
    each is missing its iterable expression.

lowering-each-missing-body =
    each is missing its body block.

lowering-select-missing-scrutinee =
    select is missing its scrutinee expression.

lowering-select-when-invalid =
    when arm is invalid.
lowering-select-when-invalid.help =
    A when must contain (pattern, block).

lowering-select-otherwise-invalid =
    otherwise arm is invalid.
lowering-select-otherwise-invalid.help =
    otherwise must contain exactly one block.

lowering-give-value-invalid =
    give value is invalid.

lowering-emit-value-invalid =
    emit value is invalid.

lowering-defer-missing-action =
    defer is missing its action.
lowering-defer-missing-action.help =
    Provide an action node (call/set/emit) as the first child.

lowering-defer-action-invalid =
    defer action is invalid: { $action }.
lowering-defer-action-invalid.help =
    Supported defer actions: call(expr), set(target, op, value), emit(expr).

lowering-assert-missing-cond =
    assert is missing its condition.

lowering-assert-message-invalid =
    assert message is invalid.


### Expression lowering

lowering-unknown-expr-tag =
    Unknown expression tag: { $tag }.

lowering-expr-missing-text =
    Expression node is missing its textual payload.

lowering-expr-literal-invalid =
    Literal is invalid: { $lit }.

lowering-expr-name-invalid =
    Name reference is invalid: { $name }.

lowering-expr-path-invalid =
    Path reference is invalid: { $path }.

lowering-expr-call-missing-callee =
    call expression is missing its callee.

lowering-expr-call-arg-invalid =
    call argument is invalid.

lowering-expr-index-arity =
    index expression requires (base, index).

lowering-expr-field-arity =
    field expression requires (base) and a field name.

lowering-expr-unary-arity =
    unary expression requires (op, expr).

lowering-expr-unary-op-invalid =
    unary operator is invalid: { $op }.
lowering-expr-unary-op-invalid.help =
    Supported unary operators: '-', 'not'.

lowering-expr-binary-arity =
    binary expression requires (left, op, right).

lowering-expr-binary-op-invalid =
    binary operator is invalid: { $op }.

lowering-expr-list-item-invalid =
    list literal contains an invalid item.

lowering-expr-map-entry-invalid =
    map literal entry is invalid.
lowering-expr-map-entry-invalid.help =
    Each map entry must have exactly (key, value).

lowering-expr-pack-item-invalid =
    pack literal contains an invalid item.


### Pattern lowering

lowering-unknown-pattern-tag =
    Unknown pattern tag: { $tag }.

lowering-pattern-variant-invalid =
    Variant pattern is invalid.
lowering-pattern-variant-invalid.help =
    Provide a type path, a case name, and optional payload patterns.

lowering-pattern-tuple-item-invalid =
    Tuple pattern contains an invalid item.

lowering-pattern-list-item-invalid =
    List pattern contains an invalid item.


### Attribute normalization / targets

lowering-attr-unknown =
    Unknown attribute: { $name }.

lowering-attr-disallowed-target =
    Attribute { $name } is not allowed on { $target }.
lowering-attr-disallowed-target.help =
    Move the attribute to a compatible declaration or remove it.

lowering-attr-duplicate =
    Duplicate attribute: { $name }.
lowering-attr-duplicate.help =
    Remove the duplicate or merge the attribute arguments.

lowering-attr-args-invalid =
    Attribute { $name } has invalid arguments.

lowering-attr-cfg-invalid =
    cfg attribute is invalid.

lowering-attr-cfg-unknown-key =
    cfg attribute uses unknown key: { $key }.
lowering-attr-cfg-unknown-key.help =
    If strict cfg is enabled, only registered keys are allowed (e.g. os, arch, feature).


### Recovery / internal

lowering-internal-unreachable =
    Internal error: unreachable lowering path.

lowering-internal-missing-child =
    Internal error: expected child { $index } but it was missing.

lowering-recovery-insert-hole =
    Inserted a hole node for recovery.

lowering-recovery-continue =
    Continuing lowering after error(s).

lowering-summary =
    Lowering completed: { $items } item(s), { $stmts } stmt(s), { $exprs } expr(s), { $errors } error(s).
