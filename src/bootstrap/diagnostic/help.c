#include "help.h"
#include "registry.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Vitte diagnostic contextual-help subsystem.
 *
 * This module turns structured diagnostic information into conservative,
 * actionable remediation guidance.
 *
 * The diagnostic registry remains authoritative for diagnostic identity and
 * metadata. This file owns remediation policy.
 *
 * Principles:
 *
 *   - stable public diagnostic codes drive generic remediation policy;
 *   - explicit HELP annotations always have priority;
 *   - structured semantic information refines generic policy;
 *   - source excerpts are used only when they are safe and exact;
 *   - no source-specific fact is invented;
 *   - no automatic conversion is recommended unless its intent is known;
 *   - no fix-it is fabricated here;
 *   - output remains deterministic;
 *   - malformed diagnostics must not crash help formatting;
 *   - UTF-8 source text must never be split;
 *   - terminal control characters are never copied into generated help;
 *   - C17.
 */

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_HELP_EXCERPT_CAPACITY       ((size_t)160u)
#define VITTE_HELP_REASON_LIMIT           ((size_t)192u)
#define VITTE_HELP_SUBJECT_LIMIT          ((size_t)128u)
#define VITTE_HELP_TYPE_LIMIT             ((size_t)128u)
#define VITTE_HELP_CONTRACT_LIMIT         ((size_t)192u)
#define VITTE_HELP_EXPECTED_TOKEN_LIMIT   ((size_t)96u)

/* ========================================================================= */
/* Static remediation registry                                               */
/* ========================================================================= */

typedef struct vitte_diagnostic_help_rule {
    const char *code;
    const char *help;
} vitte_diagnostic_help_rule_t;

/*
 * Public IDs are stable.
 *
 * Keep exactly one generic remediation policy for every registered diagnostic
 * that is intended to expose user-facing help.
 *
 * Registry coverage tests should fail when a newly registered public
 * diagnostic has no corresponding policy.
 *
 * Policies intentionally remain generic. Source-specific corrections are
 * produced later from structured diagnostic fields.
 */
static const vitte_diagnostic_help_rule_t vitte_help_rules[] = {
    {
        "E0000",
        "Capture the failing command and compiler version; this failure has no identified cause and needs a compiler bug report."
    },
    {
        "E0001",
        "Rerun with VITTE_BACKTRACE=1 and report the failing input, command and compiler version."
    },
    {
        "E0002",
        "Capture the violated invariant with VITTE_BACKTRACE=1; report the input that triggers this compiler defect."
    },
    {
        "E0003",
        "Capture the impossible state with VITTE_BACKTRACE=1 and include the smallest reproducing input in a compiler bug report."
    },
    {
        "E0004",
        "Check available memory and process resource limits; retry with fewer compilation jobs if the system is under memory pressure."
    },
    {
        "E0005",
        "Resolve the source path relative to the working directory and ensure that it names an existing, readable regular file."
    },
    {
        "E0006",
        "Save the source as UTF-8 and replace the invalid byte sequence at the reported offset."
    },
    {
        "E0007",
        "Register the original source before mapping its spans; preserve the generated file and source mapping in a compiler bug report."
    },
    {
        "E0008",
        "Report the diagnostic construction failure with its original message, source span and compiler version."
    },
    {
        "E0009",
        "Fix the first reported error before rerunning; later diagnostics were suppressed after the configured diagnostic limit was reached."
    },
    {
        "E0010",
        "Select terminal diagnostic output to inspect the formatting failure; report the failing output format and reproducing input."
    },
    {
        "E0011",
        "Correct the option named in the diagnostic and use `vitte --help` to verify its accepted values and combinations."
    },
    {
        "E0012",
        "Resolve the first diagnostic emitted by the failed compiler phase; if none was emitted, report the phase and reproducing command."
    },
    {
        "E0013",
        "Identify the unavailable compiler component or external tool from the reported cause and install, configure or report that component."
    },
    {
        "E0014",
        "The frontend API rejected this source without returning parser or semantic details. Run `vitte check FILE --diagnostic=terminal` to locate the source-level cause; if the CLI accepts the same file, reduce the input and report the API and compiler versions."
    },

    {
        "E0101",
        "Remove the invalid character or place it inside a quoted string when it is intended to be text."
    },
    {
        "E0102",
        "Close the string opened with `\"` before the line ends; escape a quote that belongs inside the string."
    },
    {
        "E0103",
        "Use `\\\\`, `\\'`, `\\\"`, `\\0`, `\\a`, `\\b`, `\\f`, `\\n`, `\\r`, `\\t`, `\\v`, `\\xNN` (2 hex digits), `\\uNNNN` (4), or `\\UNNNNNNNN` (8); escape the backslash when it is literal."
    },
    {
        "E0104",
        "Use digits valid for the literal's base and place separators only where the numeric grammar permits them; verify the prefix and suffix."
    },
    {
        "E0105",
        "Keep the integer magnitude within the supported u64 range [0, 18446744073709551615]; an explicitly annotated target type may impose a narrower range."
    },
    {
        "E0106",
        "Close the block comment opened with `/*` using `*/`, including each nested comment that remains open."
    },
    {
        "E0107",
        "Start the identifier with a permitted identifier-start character and remove characters that cannot occur in an identifier."
    },
    {
        "E0108",
        "Replace the malformed Unicode sequence with valid UTF-8 source text before compiling again."
    },
    {
        "E0109",
        "Use decimal digits or a `0b`, `0o` or `0x` prefix with valid digits; place underscores singly between digits and omit suffixes."
    },
    {
        "E0110",
        "Use a decimal mantissa with optional `e`/`E` exponent, or a hexadecimal mantissa with required `p`/`P` exponent."
    },
    {
        "E0111",
        "Put exactly one Unicode scalar value or one supported escape sequence between the single quotes."
    },
    {
        "E0112",
        "Place a single underscore between two digits; do not start, end, or repeat a separator."
    },
    {
        "E0113",
        "Use only `0`-`1` after `0b`, `0`-`7` after `0o`, and hexadecimal digits after `0x`; include at least one digit."
    },
    {
        "E0114",
        "Remove the suffix; Vitte numeric literals do not define integer or floating-point suffixes."
    },
    {
        "E0115",
        "After `e`/`E` or `p`/`P` and an optional sign, provide one or more decimal digits."
    },
    {
        "E0116",
        "Check the reported punctuation run against Vitte's operators; use the complete spelling such as `->`, `=>`, `==`, `!=`, `<=`, `>=`, `&&`, `||`, `??`, `::`, `..`, or the supported compound assignments."
    },

    {
        "E0201",
        "Insert the expected delimiter, keyword or syntactic element before the token reported as found."
    },
    {
        "E0202",
        "Remove the unexpected token or finish the preceding construct before beginning this one."
    },
    {
        "E0203",
        "Provide a valid expression at this position, such as a literal, visible name, call or other expression accepted by the grammar."
    },
    {
        "E0204",
        "Introduce a declaration at this position; move executable statements into a procedure or another context that accepts statements."
    },
    {
        "E0205",
        "Provide a valid type at this position, such as a primitive type or a visible declared type."
    },
    {
        "E0206",
        "Provide a pattern compatible with the matched value before the branch body."
    },
    {
        "E0207",
        "Close the opening delimiter with its matching closing delimiter before the enclosing construct ends."
    },
    {
        "E0208",
        "Replace the closing delimiter with the delimiter matching the relevant unclosed opener."
    },
    {
        "E0209",
        "Remove this modifier or apply it only to a declaration kind that accepts it."
    },
    {
        "E0210",
        "This token is recognized, but it has no supported grammar form here. Replace it with a construct valid in this position, or use a supported declaration such as `proc`, `form`, `pick`, `const`, or `type`."
    },
    {
        "E0211",
        "Rename this item; Vitte reserved words cannot be used as identifiers."
    },
    { "E0212", "Write each module path segment as an identifier and separate segments with `::` or `/`; remove leading, repeated or trailing separators." },
    { "E0213", "Keep one top-level `space path` header in this file and remove the duplicate declaration." },
    { "E0214", "Move `space path;` before the first import or other top-level declaration." },
    { "E0215", "Use `use path;`, `use path as alias;`, `use path::*;` or `use path::{item, nested::{item}};`, then terminate the declaration with `;`." },
    { "E0216", "Add an identifier after `as`, for example `use math::vector as vectors;`." },
    { "E0217", "Add comma-separated import paths, close every brace, and put nested groups after a path such as `ops::{arithmetic::{add, sub}}`." },
    { "E0218", "Place `::*` at the end of the import, for example `use math::*;`; remove any alias, group placement or following path segment." },
    { "E0219", "Use `export path [as alias];`, `export {path, other};` or `export *;` and remove extra tokens." },
    { "E0220", "Add at least one path inside the braces, for example `export {math::add};`." },
    { "E0221", "Add an identifier after `as`, for example `export math::add as add;`." },
    { "E0222", "Remove the extra token before the required semicolon or finish the clause in the form shown by the diagnostic." },
    { "E0223", "Add one top-level `space path;` declaration before the first module import or export clause." },
    { "E0224", "Give the declaration a name, put an optional type after `:`, add `= expression`, and finish with `;`." },
    { "E0225", "Add `= expression` after the name and optional type, for example `const LIMIT: int = 10;`." },
    { "E0226", "Complete the type name, each bound or aliased type, and the final `;`." },
    { "E0227", "Give the opaque type a name, use `= Type` for an optional alias, and omit bounds." },
    { "E0228", "Write a named form with `{}` and fields in the form `name: Type`." },
    { "E0229", "Rename this field; every field in one form must have a unique name." },
    { "E0230", "Separate fields with `,` or `;`, for example `form Point { x: int, y: int }`." },
    { "E0231", "Write a named pick with braces and comma-separated variants, for example `pick Option { None, Some(int) }`." },
    { "E0232", "Rename this variant; each variant name must be unique within its pick." },
    { "E0233", "Put valid comma-separated types inside the variant parentheses, for example `Some(int, string)`; `()` is also valid." },
    { "E0234", "Write `trait Name { ... }`, with optional generic parameters and a `where` clause before the block." },
    { "E0235", "Write an inherent implementation as `impl Type { ... }` or a trait implementation as `impl Trait for Type { ... }`." },
    { "E0236", "Add the type after `for`, for example `impl Display for Point { ... }`." },
    { "E0237", "Use one trait type, `for`, one target type, then an optional `where` clause and the implementation block." },
    { "E0238", "Write `proc name(parameters)`, then an optional `-> Type`, optional constraints, and a body or `;`." },
    { "E0239", "Enclose parameters in `(...)`; each parameter needs a pattern, `:`, a type, commas between entries, and a closing `)`." },
    { "E0240", "Put a valid type after `->`, for example `proc size() -> int;`." },
    { "E0241", "Use `async`, `unsafe` or `comptime` at most once and only before a procedure declaration." },
    { "E0242", "A normal procedure needs a block or `;`; an `extern` or `intrinsic` procedure must end with `;` and has no body." },
    { "E0243", "Write `extern [\"ABI\"] proc name(parameters) [-> Type] [where ...] [requires ...;] [ensures ...;];`, for example `extern \"C\" proc write(fd: i32, data: *const u8) -> isize;`." },
    { "E0244", "Write `intrinsic name(parameters) [-> Type] [where ...] [requires ...;] [ensures ...;];` without an ABI literal or body." },
    { "E0245", "Use a nonempty quoted ABI name after `extern`, for example `extern \"C\" proc write();`." },
    { "E0246", "Write `<T, U>` with comma-separated parameters and a closing `>`; a trailing comma is allowed." },
    { "E0247", "Write a const parameter as `const NAME`, optionally followed by a type bound, for example `const N: usize`." },
    { "E0248", "Add a valid type after `:` or `+`, for example `T: Display + Debug`." },
    { "E0249", "Write comma-separated predicates after `where`, for example `proc bounded<const N: usize>() where N > 0 { ... }`; give each predicate a valid relation and complete right-hand expression." },
    { "E0250", "Add a boolean expression and its semicolon, for example `proc positive(value: int) -> int requires value > 0; { return value; }`; `requires` checks callers before the body runs." },
    { "E0251", "Add a boolean expression and its semicolon, for example `proc positive(value: int) -> int ensures result >= 0; { return value; }`; `ensures` checks the result after return." },
    { "E0252", "Write `macro name` with an optional parameter list and a body, for example `macro log(value: string) { ... }`." },
    { "E0253", "Use `(...)` with comma-separated `pattern: Type` parameters and a closing `)`." },
    { "E0254", "Write `test { ... }`, `test test_name { ... }`, or `test \"description\" { ... }`." },
    { "E0255", "Use at most one visibility modifier (`pub` or prefix `export`) before procedure modifiers; write export clauses without declaration modifiers." },
    { "E0256", "Write an attribute as `#[name]` or `#![name]`; include the opening `[` and closing `]` and close any argument parentheses." },
    { "E0257", "Provide an identifier or keyword after `#[` or `#![` and after each `::` in a qualified attribute name." },
    { "E0258", "Use comma-separated names or literals in parentheses; write named arguments as `name = literal`." },
    { "E0259", "Add a literal after `=`, for example `#[cfg(enabled = true)]`." },
    { "E0260", "Use an integer, float, string, character, boolean, or `null` literal as the attribute value." },
    { "E0261", "Move this declaration to a declaration position, such as the top level or a trait/implementation declaration block." },
    { "E0262", "Move this statement into a block that accepts statements, such as a procedure body." },
    { "E0263", "Move this expression into a statement or expression context; declarations require a declaration position." },
    { "E0264", "Write `proc(Type, Type)`, optionally followed by `-> ReturnType`; separate parameters with commas and close the list with `)`." },
    { "E0265", "Write `&Type`, `&mut Type`, `ref Type` or `ref mut Type`, with a complete type after the reference marker." },
    { "E0266", "Write `*Type`, `*mut Type`, `*const Type`, `*volatile Type` or `*const volatile Type`; provide one complete pointee type." },
    { "E0267", "Write `[Type; length]` or `[length] Type`, with a valid length expression, closing `]` and element type." },
    { "E0268", "Write a slice as `[ElementType]` and include a complete element type before the closing `]`." },
    { "E0269", "Write `?Type` and put a complete type after `?`." },
    { "E0270", "Use nonempty path segments separated by `::` or `/`; use `::` instead of `:`, add a segment after every separator and remove repeated separators." },
    { "E0271", "Use zero or more comma-separated types in `<Type, Type>` or `[Type, Type]`; a trailing comma and an empty list are allowed, and the closing delimiter must match." },
    { "E0272", "Write `let [mut] pattern [: Type] [= expression];`. Add a type annotation when there is no initializer and the binding type cannot be inferred." },
    { "E0273", "Write `set target assignment_operator expression;` and provide a complete target, value and terminating semicolon." },
    { "E0274", "Use one of `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=` or `>>=` after the assignment target." },
    { "E0275", "Write `return;` or `return expression;`. The final return in a block may omit `;` before `}` or end of file; procedure placement and return type are checked semantically." },
    { "E0276", "Write `break;` or `continue;` with a semicolon. A jump must appear inside a loop within a procedure." },
    { "E0277", "Write `defer { ... }` or `defer expression;`; the expression form requires its terminating semicolon." },
    { "E0278", "Write `assert(condition);` or `assert condition;` with a complete expression and a final semicolon." },
    { "E0279", "Write `unsafe { ... }` and close the block with `}`." },
    { "E0280", "Write `asm(operand, ...);` or `asm(\"template\");`; close the operand list and terminate the statement with `;`." },
    { "E0281", "Use `if condition { ... } [elif condition { ... }]* [else { ... }]`, `while condition { ... }`, `loop { ... }`, or `for pattern in values { ... }`; put `else` after an if/elif chain and use it only before a block or chained conditional." },
    { "E0282", "Write a match statement with a subject and block bodies, for example `proc choose(option: Maybe) -> int { match option { case Maybe::Some(item) => { return item; }, case Maybe::None => { return 0; } } }`. Separate complete arms with commas." },
    { "E0283", "Write `[case] pattern [if guard] => body`, separate adjacent arms with commas, and use a complete expression, statement or block as the body." },
    { "E0284", "Use a path or binding, literal, `Path::Variant`, `Variant(pattern, ...)`, `Path { field: pattern, .. }`, `(pattern, ...)`, or `[pattern, ...]`; separate pattern items with commas." },
    { "E0285", "Write `lower..upper` or `lower..=upper`; both endpoints must be integer, float or character literals of the same kind." },
    { "E0286", "Write `target = value` or use a supported compound assignment operator; provide both a complete target and right-hand expression." },
    { "E0287", "Write `value ?? fallback` and provide a complete expression on both sides of `??`." },
    { "E0288", "Write both operands around `and`, `or`, `&&` or `||`; parenthesize nested conditions when needed." },
    { "E0289", "Write both operands around `&`, `|` or `^`; use a complete expression on each side." },
    { "E0290", "Write a complete left and right expression around `==`, `!=`, `<`, `<=`, `>` or `>=`." },
    { "E0291", "Write a value and shift count around `<<` or `>>`." },
    { "E0292", "Write complete operands around `+`, `-`, `*`, `/` or `%`; add parentheses to make grouping clear." },
    { "E0293", "Write `start..end` for an exclusive range or `start..=end` for an inclusive range, with a complete expression at each endpoint." },
    { "E0294", "Put one complete operand after the unary operator, for example `-value`, `not ready`, `&value` or `*pointer`." },
    { "E0295", "Write `value as Type` and provide a complete type after `as`." },
    { "E0296", "Complete the suffix: use `callee(arguments)`, `value[index]`, `value.field`, `value?`, or `Type { field: value }`; generic form and variant suffixes need nonempty, well-formed type arguments." },
    { "E0297", "Write `if condition { value } else { fallback }`; an if expression needs an else branch, and `elif` may continue the chain." },
    { "E0298", "Close the block with `}`; place complete statements before the optional final expression." },
    { "E0299", "Write `[]` or `[expression, ...]`; provide complete elements separated by commas and close the array with `]`." },

    {
        "E0300",
        "Change the import to match the resolved file's `space` declaration, or change that declaration to match the imported path."
    },
    {
        "E0301",
        "Check the exact module path and the search roots listed in the diagnostic; add the source file under one of those roots or correct the import."
    },
    {
        "E0302",
        "Import an exported symbol or change the declaration's visibility when external access is intentionally part of the module API."
    },
    {
        "E0303",
        "Break the reported import cycle by moving shared declarations into a module that does not depend back on either side of the cycle."
    },
    {
        "E0304",
        "Remove the repeated import. If two imports would bind different declarations to the same local name, give the imported items distinct aliases."
    },
    {
        "E0305",
        "Install the declared package into the project or user module directory, or add its package directory to `VITTE_MODULE_PATH`."
    },
    {
        "E0306",
        "Select a package version satisfying the declared constraint, or update the constraint after checking API compatibility."
    },
    {
        "E0307",
        "Preserve the import graph, failing validation detail and compiler version; this structural failure should be reported as a compiler defect."
    },
    { "E0308", "Write `sizeof(Type)` or `sizeof(expression)` with one complete operand and matching parentheses." },
    { "E0309", "Write `alignof(Type)` with a complete type and matching parentheses." },
    { "E0310", "Write `null()` or `null(expression, ...)` and close the argument list with `)`." },
    { "E0311", "Provide complete expressions separated by commas. A trailing comma is allowed, but empty items are invalid; a missing-comma insertion is suggested only when the next token clearly starts another item." },

    {
        "E0401",
        "Declare or import the unresolved name in this scope and verify its spelling against visible declarations."
    },
    {
        "E0402",
        "Rename this declaration or remove the earlier declaration with the same name in this scope."
    },
    {
        "E0403",
        "Use the candidate locations to identify the intended declaration. Replace conflicting glob imports with explicit grouped imports and distinct aliases, or qualify each use through its module."
    },
    {
        "E0404",
        "Add `pub` to the declaration in its defining module, or add `export name;` there if the symbol belongs in that module's API."
    },
    {
        "E0405",
        "Move the export clause to root or module scope. Inside a trait or impl, use `pub` on an individual declaration."
    },
    {
        "E0406",
        "Give every binding introduced by this pattern or declaration a distinct name."
    },
    {
        "E0407",
        "Export a declaration defined in this module. Re-exports are unsupported; add `pub` to the declaration in its defining module, then import it with `use module::{name};`."
    },

    {
        "E0501",
        "Make the expression and its consumer agree on a type. If the consumer's type is intended, produce or validate a value of that type; if the expression's type is intended, correct the declaration or operation that requires the other type. Convert only when the value and precision remain valid."
    },
    {
        "E0502",
        "Choose the intended type explicitly, for example `let count: i32 = 0;` for a signed count or `let count: usize = 0;` for an index, or provide an initializer with a uniquely known type."
    },
    {
        "E0503",
        "Use a conversion supported by both types; parse or format textual representations explicitly instead of relying on an unrelated cast."
    },
    {
        "E0504",
        "Call a procedure or callable value and check whether another declaration shadows the intended procedure."
    },
    {
        "E0505",
        "Use a value supporting indexed access or access this value through operations provided by its declared type."
    },
    {
        "E0506",
        "Use a member declared by the receiver's type and verify both the receiver type and member spelling."
    },
    {
        "E0507",
        "Change the type argument or implementation so that it satisfies the constraint named by the diagnostic."
    },
    {
        "E0508",
        "Provide the complete type definition before performing an operation that requires its size, layout or members."
    },
    {
        "E0509",
        "Break the recursive by-value type cycle with a supported indirection or redesign the recursive representation."
    },
    {
        "E0510",
        "Declare or import the type in this scope and verify the spelling and visibility of its declaration."
    },
    {
        "E0511",
        "Use a type name at this position instead of a value, variable or procedure name."
    },
    {
        "E0512",
        "Choose an operator defined for these operand types or explicitly convert the operands to compatible types."
    },
    {
        "E0513",
        "Assign only to a writable mutable location such as a mutable binding, field or index."
    },
    {
        "E0514",
        "Use an index of the type required by this collection and verify that the index expression represents an element position."
    },
    {
        "E0515",
        "Use a boolean condition, normally by writing an explicit comparison; Vitte does not infer truthiness from unrelated value types."
    },
    {
        "E0516",
        "Use this name in a type position, or construct a value with the type's supported constructor syntax."
    },

    {
        "E0601",
        "Compare this call with the procedure signature: add or remove arguments if the call is wrong, or change the parameter list if the declared API is wrong."
    },
    {
        "E0602",
        "Pass an expression satisfying the declared parameter type at this argument position."
    },
    {
        "E0603",
        "Compare the argument count and types with the available signatures and select a callable candidate that accepts them."
    },
    {
        "E0604",
        "Qualify the procedure or make argument types explicit so that exactly one callable candidate matches."
    },
    {
        "E0605",
        "Return an expression satisfying the procedure's declared result type or correct the result declaration when the API is wrong."
    },
    {
        "E0606",
        "Remove the incompatible call modifier or select a procedure supporting the required calling context."
    },
    {
        "E0607",
        "Place `return` inside a procedure body so it has a result to return."
    },
    {
        "E0608",
        "Use an irrefutable parameter binding such as `value: Type`; move refutable matching into the procedure body and use only a parameter mode documented by Vitte."
    },

    {
        "E0700",
        "Use a well-typed boolean expression that is valid in this contract's declaration context."
    },
    {
        "E0701",
        "Adjust the call arguments or caller state so the requires condition holds before entering the procedure."
    },
    {
        "E0702",
        "Correct the returned value or resulting state so every successful return satisfies the ensures condition."
    },
    {
        "E0703",
        "Restore the invariant before this program point and preserve it on every control-flow path reaching the check."
    },
    {
        "E0704",
        "Make the requires clause a boolean expression referring only to values available before the call."
    },
    {
        "E0705",
        "Make the ensures clause a boolean expression valid after the procedure has produced its result."
    },
    {
        "E0706",
        "`old(...)` needs a value captured at procedure entry. Entry-state snapshots are not available for this reference; remove it or express the condition using values supported by the contract."
    },
    {
        "E0707",
        "Use `result` only in a contract context where a procedure result exists."
    },
    {
        "E0708",
        "Provide the missing precondition or simplify the assertion so the checker has enough information to prove the required relationship."
    },
    {
        "E0709",
        "Remove or correct contradictory contract clauses so at least one valid program state can satisfy the contract."
    },
    {
        "E0710",
        "Move state changes and I/O out of the contract and compute the condition only from permitted side-effect-free values."
    },
    {
        "E0711",
        "Replace the call inside the contract with an operation permitted during contract evaluation."
    },
    {
        "E0712",
        "Align the implementation contract with the declared API: do not strengthen caller requirements or weaken promised results."
    },
    {
        "E0713",
        "Inspect the values reported by the failed runtime contract check and correct the caller or implementation violating the condition."
    },
    {
        "E0714",
        "Report the contract expression, reproducing input and compiler version because contract analysis failed internally."
    },

    {
        "E0801",
        "Choose a generic argument satisfying the named bound or provide the implementation required by that bound."
    },
    {
        "E0802",
        "Supply one type or value argument for every required generic parameter in declaration order."
    },
    {
        "E0803",
        "Match each supplied argument to the declared parameter kind and position; a type argument cannot fill a const parameter."
    },
    {
        "E0804",
        "Make the arguments agree on one concrete type for each generic parameter, or split the call into separately typed values."
    },
    {
        "E0810",
        "Implement the required trait for this type or use a type that already provides the required implementation."
    },
    {
        "E0811",
        "Disambiguate the trait relation so exactly one implementation matches the type and generic arguments."
    },
    {
        "E0812",
        "Remove or specialize the overlapping implementation so a trait/type relation resolves to exactly one implementation."
    },
    {
        "E0813",
        "Add the required method or associated member to this implementation, or provide a default in the trait."
    },
    {
        "E0814",
        "Match the trait member's name, parameter types, return type and allowed declaration form."
    },
    {
        "E0815",
        "Use `impl Trait for Type` for a trait implementation, or `impl Type` for an inherent implementation."
    },
    {
        "E0816",
        "Use a declared generic parameter on the left of `:` and one or more visible trait bounds on the right."
    },
    {
        "E0817",
        "Qualify the call through the intended trait or remove the competing visible method implementation."
    },

    {
        "E0901",
        "Preserve the generated C and the external compiler's first error, then report the Vitte construct that produced invalid C17."
    },
    {
        "E0902",
        "Reduce the failing construct to a reproducer and report the C17 lowering failure together with its backend detail."
    },
    {
        "E0903",
        "Rewrite the construct using features supported by the selected backend or select a backend that implements the required feature."
    },
    {
        "E0904",
        "Check the reported phase and command, identify the selected compiler (`CC` in native host-tools or `cc` on PATH in the bootstrap fallback), and confirm it accepts C17. Inspect its first output: errors in generated C may indicate a backend defect, while missing headers or SDK files are toolchain configuration issues."
    },
    {
        "E0905",
        "Preserve the generated C and original Vitte source together and report the incorrect source mapping."
    },
    {
        "E0906",
        "Choose a writable output path, check the parent directory permissions and available storage, then retry the output operation."
    },
    {
        "E0907",
        "Use the reported link command and first linker error to check the selected compiler driver (`CC` in native host-tools or `cc` on PATH in the bootstrap fallback), target format, SDK/sysroot, library search paths and required symbols. Keep link failures separate from C compilation errors."
    },
    {
        "E0908",
        "Inspect the generated program's exit status and runtime output; compilation and linking completed before this failure."
    },
    {
        "E0909",
        "Verify the selected C17 backend and its configuration values, then report the invalid option and reproducing command."
    },

    {
        "E1001",
        "Report the invalid IR instruction and its originating source because compiler-generated IR must satisfy the verifier."
    },
    {
        "E1002",
        "Report the source construct that failed IR lowering together with the failing phase and compiler version."
    },
    {
        "E1003",
        "Preserve the verifier's first invariant failure and the corresponding IR dump in a compiler bug report."
    },
    {
        "E1004",
        "Use a source construct supported by the IR lowerer or select a compilation path that implements this valid construct."
    },
    {
        "E1010",
        "Report the invalid HIR construct and its originating source so frontend lowering can be corrected."
    },
    {
        "E1011",
        "Report the source construct and compiler version; the native frontend failed while building or validating its high-level representation."
    },
    {
        "E1012",
        "Use a source construct supported by the HIR lowerer or select a compilation path that implements this valid construct."
    },

    {
        "E1101",
        "Add `return` with a value satisfying the declared result type on every reachable path, including fallthrough."
    },
    {
        "E1102",
        "Place `break` inside a loop body to exit that loop."
    },
    {
        "E1103",
        "Place `continue` inside a loop body to start its next iteration."
    },
    {
        "E1104",
        "Correct the preceding branch, termination or control-flow condition so this operation is reachable when it is intended to execute."
    },
    {
        "E1105",
        "Keep deferred work within its enclosing scope: remove `return`, `break`, `continue`, early-exit `?`, and nested `defer` from the deferred action."
    },

    {
        "E1201",
        "Verify that the referenced allocation remains alive and that the accessed range lies inside the allocation."
    },
    {
        "E1202",
        "Handle the null case before dereferencing or provide a valid initialized reference."
    },
    {
        "E1203",
        "Use a pointer to the required pointee type and verify the pointee representation before performing an explicit conversion."
    },
    {
        "E1204",
        "Allocate or offset the storage so the accessed value satisfies the required alignment."
    },
    {
        "E1205",
        "Keep the referenced value alive for at least as long as the reference, and use a direct local or parameter borrow whose origin the bootstrap can track."
    },
    {
        "E1206",
        "End the existing borrow before creating an overlapping exclusive reference; use shared references only when no mutation is required."
    },
    {
        "E1207",
        "Wait until the incompatible reference is no longer used, or access the value through the active reference with its permitted mutability."
    },
    {
        "E1208",
        "End every reference to the value before moving it, or move a different local value that is not currently borrowed."
    },
    {
        "E1209",
        "Initialize the binding again before using it, or remove the move if the original value is still needed."
    },

    {
        "E1301",
        "Review the operation's documented safety requirements and prove each invariant at this call site. Prefer a safe API when it provides the needed behavior; mark a context unsafe only when the caller can uphold those requirements."
    },
    {
        "E1302",
        "Match assembly instructions, operands and constraints to the selected target architecture."
    },
    {
        "E1303",
        "Use the intrinsic's required argument types and compile-time values on a target that supports the intrinsic."
    },

    {
        "E1401",
        "Choose an ABI supported by this target and match the foreign declaration's calling convention."
    },
    {
        "E1402",
        "Use a foreign-compatible representation and explicitly marshal values whose Vitte representation cannot cross the foreign interface."
    },
    {
        "E1403",
        "Match the foreign variadic signature and apply the ABI-required argument promotions to every trailing argument."
    },
    {
        "E1404",
        "Match the link name to the actual exported foreign symbol and remove conflicting foreign declarations."
    },

    {
        "E1501",
        "Replace runtime-dependent values with compile-time constants or move this computation out of the constant-evaluation context."
    },
    {
        "E1502",
        "This compile-time arithmetic operation overflows `i64` in the underlined expression. Reduce the operands or use a wider representable type."
    },
    {
        "E1503",
        "The divisor of this compile-time division evaluates to zero in the underlined expression. Change the divisor or guard the operation."
    },
    {
        "E1504",
        "Break the constant dependency cycle by giving at least one value an independent initializer."
    },
    {
        "E1505",
        "Reduce compile-time recursion or work and verify termination before increasing an evaluation limit."
    },
    {
        "E1506",
        "The divisor of this compile-time remainder evaluates to zero in the underlined expression. Change the divisor or guard the operation."
    },

    {
        "E1601",
        "Add an arm for each listed missing variant or representative uncovered pattern, or use an unguarded `_` arm when all remaining values should be handled uniformly."
    },
    {
        "E1602",
        "Remove the already covered pattern or place a more specific pattern before the branch that subsumes it."
    },
    {
        "E1603",
        "Use a pattern compatible with the scrutinee type and correct incompatible variants or literals."
    },
    {
        "E1604",
        "Make every alternative introduce the same binding names with compatible binding types."
    },

    {
        "E1701",
        "Move the asynchronous operation into a compatible async procedure or use the supported synchronous operation."
    },
    {
        "E1702",
        "Await a task-compatible result instead of an already computed ordinary value."
    },
    {
        "E1710",
        "Transfer an owned send-compatible value or use a supported synchronization mechanism between concurrent tasks."
    },
    {
        "E1711",
        "Move the forbidden blocking or suspending operation outside the critical section."
    },

    {
        "E1801",
        "Correct the macro arguments or expansion so it produces syntax valid at this invocation site."
    },
    {
        "E1802",
        "Break recursive macro expansion or reduce its nesting before increasing any expansion limit."
    },
    {
        "E1810",
        "Capture the failing compiler pass, its input and compiler version in a compiler bug report."
    },
    {
        "E1811",
        "Preserve the representation immediately before and after the failing pass together with the verifier's first error."
    },

    {
        "E1901",
        "Compare the requested triple with the reported host architecture, platform and pointer width. Check the selected compiler's default target (for example with `-dumpmachine`) and its SDK/sysroot; this C17 path does not forward `--target`."
    },
    {
        "E1902",
        "Compare the requested architecture with the target triple and the compiler's default target. Select a compiler configured for that architecture only when the backend can pass or otherwise honor the target."
    },
    {
        "E1903",
        "Check the requested CPU feature against the selected architecture and compiler version; remove only a feature confirmed unavailable or choose a toolchain that supports it."
    },
    {
        "E1904",
        "Compare the requested pointer width with the width supported by this compilation mode and with FFI declarations. Use a target-aware backend when the requested width differs from the host build."
    },
    {
        "E1905",
        "Check the target byte order reported by the toolchain and the operation's documented support. Preserve the source representation and inspect target-specific behavior before changing code."
    },

    {
        "W0001",
        "Remove the unused local binding or use its value in the computation that requires it."
    },
    {
        "W0002",
        "Use the parameter in the procedure body or remove it and update callers when it is not part of the required API."
    },
    {
        "W0003",
        "Remove the import when no declaration from it is required."
    },
    {
        "W0004",
        "Handle the returned value, including any error result, before discarding it intentionally."
    },
    {
        "W0010",
        "Replace the deprecated declaration with its documented successor."
    },
    {
        "W0020",
        "Remove statements after the unconditional exit or correct the control flow that makes them unreachable."
    },
    {
        "W0030",
        "Rename the inner binding when hiding the outer declaration is unintended."
    },
    {
        "W0040",
        "Verify that the implicit conversion preserves the intended value; use compatible types or a deliberate checked conversion."
    },
    {
        "W0041",
        "Verify the source value's range before conversion and use a wider representation when significant information would be lost."
    },
    {
        "W0042",
        "Check for negative or out-of-range values before changing signedness."
    },
    {
        "W0050",
        "Remove the cast because the expression already has the requested type."
    },
    {
        "W0051",
        "Remove the redundant condition or correct the expression that makes its result constant."
    },
    {
        "W0052",
        "Review the earlier arm or narrow this pattern so the same value is not handled by both arms."
    },
    {
        "W0060",
        "Check both operands and the comparison operator for a copied name, incompatible range or unintended comparison."
    },
    {
        "W0070",
        "Use a stable foreign representation and verify field offsets, size and alignment against the foreign declaration."
    },
    {
        "W0080",
        "Keep the runtime contract check or provide enough constraints for the compiler to prove the condition statically."
    },
    {
        "W0090",
        "Inspect the reported I/O or compiler operation and restore the required resource before relying on its output."
    }
};

#define VITTE_HELP_RULE_COUNT \
    (sizeof(vitte_help_rules) / sizeof(vitte_help_rules[0]))

/* ========================================================================= */
/* Small helpers                                                             */
/* ========================================================================= */

static bool
vitte_help_text_present(
    const char *text
)
{
    return text != NULL &&
           text[0] != '\0';
}

static size_t
vitte_help_min_size(
    size_t left,
    size_t right
)
{
    return left < right
        ? left
        : right;
}

static bool
vitte_help_ascii_equal_case_insensitive(
    const char *left,
    const char *right
)
{
    unsigned char a;
    unsigned char b;

    if (left == NULL ||
        right == NULL) {
        return left == right;
    }

    while (*left != '\0' &&
           *right != '\0') {

        a = (unsigned char)*left;
        b = (unsigned char)*right;

        if (tolower(a) !=
            tolower(b)) {
            return false;
        }

        left++;
        right++;
    }

    return *left == '\0' &&
           *right == '\0';
}

static bool
vitte_help_code_equal(
    const char *left,
    const char *right
)
{
    return vitte_help_ascii_equal_case_insensitive(
        left,
        right
    );
}

/* ========================================================================= */
/* Bounded output builder                                                    */
/* ========================================================================= */

typedef struct vitte_help_builder {
    char *buffer;
    size_t capacity;
    size_t length;
    bool truncated;
    bool failed;
} vitte_help_builder_t;

static void
vitte_help_builder_init(
    vitte_help_builder_t *builder,
    char *buffer,
    size_t capacity
)
{
    if (builder == NULL) {
        return;
    }

    builder->buffer = buffer;
    builder->capacity = capacity;
    builder->length = 0u;
    builder->truncated = false;
    builder->failed = false;

    if (buffer == NULL ||
        capacity == 0u) {
        builder->failed = true;
        return;
    }

    buffer[0] = '\0';
}

static size_t
vitte_help_utf8_safe_prefix(
    const char *text,
    size_t length
)
{
    size_t result;

    if (text == NULL) {
        return 0u;
    }

    result = length;

    /*
     * If truncation ends in UTF-8 continuation bytes, back up to the leading
     * byte of the incomplete sequence.
     */
    while (result > 0u &&
           (((unsigned char)text[result] & 0xC0u) == 0x80u)) {
        result--;
    }

    if (result == length) {
        return result;
    }

    return result;
}

static void
vitte_help_builder_append_bytes(
    vitte_help_builder_t *builder,
    const char *text,
    size_t length
)
{
    size_t available;
    size_t written;

    if (builder == NULL ||
        builder->failed ||
        text == NULL ||
        length == 0u) {
        return;
    }

    if (builder->length >=
        builder->capacity - 1u) {
        builder->truncated = true;
        return;
    }

    available =
        builder->capacity -
        builder->length -
        1u;

    written =
        vitte_help_min_size(
            available,
            length
        );

    if (written < length) {
        written =
            vitte_help_utf8_safe_prefix(
                text,
                written
            );

        builder->truncated = true;
    }

    if (written == 0u) {
        return;
    }

    memcpy(
        builder->buffer + builder->length,
        text,
        written
    );

    builder->length += written;
    builder->buffer[builder->length] = '\0';
}

static void
vitte_help_builder_append(
    vitte_help_builder_t *builder,
    const char *text
)
{
    if (!vitte_help_text_present(text)) {
        return;
    }

    vitte_help_builder_append_bytes(
        builder,
        text,
        strlen(text)
    );
}

/* ========================================================================= */
/* Safe quoted fragments                                                     */
/* ========================================================================= */

static void
vitte_help_builder_append_quoted_fragment(
    vitte_help_builder_t *builder,
    const char *text,
    size_t maximum
)
{
    size_t index;
    size_t written;

    if (builder == NULL ||
        !vitte_help_text_present(text)) {
        return;
    }

    vitte_help_builder_append(
        builder,
        "`"
    );

    written = 0u;

    for (index = 0u;
         text[index] != '\0' &&
         written < maximum;
         index++) {

        unsigned char character;

        character =
            (unsigned char)text[index];

        if (character < 0x20u ||
            character == 0x7fu) {
            continue;
        }

        if (character == '`') {
            vitte_help_builder_append(
                builder,
                "\\`"
            );

            written++;
            continue;
        }

        vitte_help_builder_append_bytes(
            builder,
            &text[index],
            1u
        );

        written++;
    }

    if (text[index] != '\0') {
        vitte_help_builder_append(
            builder,
            "…"
        );
    }

    vitte_help_builder_append(
        builder,
        "`"
    );
}

/* ========================================================================= */
/* Public help policy lookup                                                 */
/* ========================================================================= */

const char *
vitte_diagnostic_help_for_code(
    const char *code
)
{
    size_t index;

    if (!vitte_help_text_present(code)) {
        return NULL;
    }

    for (index = 0u;
         index < VITTE_HELP_RULE_COUNT;
         index++) {

        if (vitte_help_code_equal(
                code,
                vitte_help_rules[index].code
            )) {

            return vitte_help_rules[index].help;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Explicit HELP annotations                                                 */
/* ========================================================================= */

bool
vitte_diagnostic_has_help(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return false;
    }

    for (index = 0u;
         index < diagnostic->annotation_count;
         index++) {

        const vitte_diagnostic_annotation_t *annotation;

        annotation =
            &diagnostic->annotations[index];

        if (annotation->kind ==
                VITTE_DIAGNOSTIC_ANNOTATION_HELP &&
            vitte_help_text_present(
                annotation->message
            )) {

            return true;
        }
    }

    return false;
}

static const char *
vitte_help_explicit_annotation(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < diagnostic->annotation_count;
         index++) {

        const vitte_diagnostic_annotation_t *annotation;

        annotation =
            &diagnostic->annotations[index];

        if (annotation->kind ==
                VITTE_DIAGNOSTIC_ANNOTATION_HELP &&
            vitte_help_text_present(
                annotation->message
            )) {

            return annotation->message;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Registry resolution                                                       */
/* ========================================================================= */

static const vitte_diagnostic_registry_entry_t *
vitte_help_registry_entry(
    const vitte_diagnostic_t *diagnostic
)
{
    const vitte_diagnostic_registry_entry_t *entry;

    if (diagnostic == NULL) {
        return NULL;
    }

    entry = NULL;

    if (vitte_help_text_present(
            diagnostic->code
        )) {

        entry =
            vitte_diagnostic_registry_find_internal(
                diagnostic->code
            );

        if (entry == NULL) {
            entry =
                vitte_diagnostic_registry_find_public(
                    diagnostic->code
                );
        }
    }

    if (entry == NULL &&
        vitte_help_text_present(
            diagnostic->short_code
        )) {

        entry =
            vitte_diagnostic_registry_find_public(
                diagnostic->short_code
            );

        if (entry == NULL) {
            entry =
                vitte_diagnostic_registry_find_internal(
                    diagnostic->short_code
                );
        }
    }

    return entry;
}

static const char *
vitte_help_public_code(
    const vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_registry_entry_t *entry
)
{
    if (entry != NULL &&
        vitte_help_text_present(
            entry->public_code
        )) {

        return entry->public_code;
    }

    if (diagnostic != NULL &&
        vitte_help_text_present(
            diagnostic->short_code
        )) {

        return diagnostic->short_code;
    }

    return NULL;
}

/* ========================================================================= */
/* Type classification                                                       */
/* ========================================================================= */

static bool
vitte_help_type_is_one_of(
    const char *name,
    const char *const *types,
    size_t count
)
{
    size_t index;

    if (!vitte_help_text_present(name)) {
        return false;
    }

    for (index = 0u;
         index < count;
         index++) {

        if (strcmp(
                name,
                types[index]
            ) == 0) {

            return true;
        }
    }

    return false;
}

static bool
vitte_help_numeric_type(
    const char *name
)
{
    static const char *const types[] = {
        "int",
        "uint",
        "i8",
        "i16",
        "i32",
        "i64",
        "i128",
        "u8",
        "u16",
        "u32",
        "u64",
        "u128",
        "isize",
        "usize",
        "float",
        "f32",
        "f64"
    };

    return vitte_help_type_is_one_of(
        name,
        types,
        sizeof(types) / sizeof(types[0])
    );
}

static bool
vitte_help_integer_type(
    const char *name
)
{
    static const char *const types[] = {
        "int",
        "uint",
        "i8",
        "i16",
        "i32",
        "i64",
        "i128",
        "u8",
        "u16",
        "u32",
        "u64",
        "u128",
        "isize",
        "usize"
    };

    return vitte_help_type_is_one_of(
        name,
        types,
        sizeof(types) / sizeof(types[0])
    );
}

static bool
vitte_help_float_type(
    const char *name
)
{
    static const char *const types[] = {
        "float",
        "f32",
        "f64"
    };

    return vitte_help_type_is_one_of(
        name,
        types,
        sizeof(types) / sizeof(types[0])
    );
}

static bool
vitte_help_boolean_type(
    const char *name
)
{
    return name != NULL &&
           strcmp(name, "bool") == 0;
}

static bool
vitte_help_string_type(
    const char *name
)
{
    return name != NULL &&
           (
               strcmp(name, "string") == 0 ||
               strcmp(name, "str") == 0
           );
}

/* ========================================================================= */
/* Source excerpt                                                            */
/* ========================================================================= */

static bool
vitte_help_utf8_sequence_valid(
    const unsigned char *bytes,
    size_t remaining,
    size_t *width
)
{
    unsigned char first;

    if (bytes == NULL ||
        remaining == 0u ||
        width == NULL) {
        return false;
    }

    first = bytes[0];

    if (first <= 0x7fu) {
        *width = 1u;
        return true;
    }

    if (first >= 0xc2u &&
        first <= 0xdfu) {

        if (remaining < 2u ||
            (bytes[1] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 2u;
        return true;
    }

    if (first == 0xe0u) {
        if (remaining < 3u ||
            bytes[1] < 0xa0u ||
            bytes[1] > 0xbfu ||
            (bytes[2] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 3u;
        return true;
    }

    if ((first >= 0xe1u &&
         first <= 0xecu) ||
        (first >= 0xeeu &&
         first <= 0xefu)) {

        if (remaining < 3u ||
            (bytes[1] & 0xc0u) != 0x80u ||
            (bytes[2] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 3u;
        return true;
    }

    if (first == 0xedu) {
        if (remaining < 3u ||
            bytes[1] < 0x80u ||
            bytes[1] > 0x9fu ||
            (bytes[2] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 3u;
        return true;
    }

    if (first == 0xf0u) {
        if (remaining < 4u ||
            bytes[1] < 0x90u ||
            bytes[1] > 0xbfu ||
            (bytes[2] & 0xc0u) != 0x80u ||
            (bytes[3] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 4u;
        return true;
    }

    if (first >= 0xf1u &&
        first <= 0xf3u) {

        if (remaining < 4u ||
            (bytes[1] & 0xc0u) != 0x80u ||
            (bytes[2] & 0xc0u) != 0x80u ||
            (bytes[3] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 4u;
        return true;
    }

    if (first == 0xf4u) {
        if (remaining < 4u ||
            bytes[1] < 0x80u ||
            bytes[1] > 0x8fu ||
            (bytes[2] & 0xc0u) != 0x80u ||
            (bytes[3] & 0xc0u) != 0x80u) {
            return false;
        }

        *width = 4u;
        return true;
    }

    return false;
}

static bool
vitte_help_source_excerpt(
    const vitte_diagnostic_t *diagnostic,
    const vitte_source_context_t *sources,
    char *text,
    size_t capacity
)
{
    const vitte_source_entry_t *source;
    const unsigned char *bytes;
    size_t length;
    size_t offset;

    if (text == NULL ||
        capacity == 0u) {
        return false;
    }

    text[0] = '\0';

    if (diagnostic == NULL ||
        sources == NULL ||
        !diagnostic->has_span ||
        diagnostic->source_id ==
            VITTE_SOURCE_ID_INVALID ||
        diagnostic->end_offset <=
            diagnostic->start_offset) {

        return false;
    }

    source =
        vitte_source_get(
            sources,
            diagnostic->source_id
        );

    if (source == NULL ||
        source->data == NULL ||
        diagnostic->start_offset >
            source->length ||
        diagnostic->end_offset >
            source->length) {

        return false;
    }

    length =
        diagnostic->end_offset -
        diagnostic->start_offset;

    if (length == 0u ||
        length >= capacity) {
        return false;
    }

    bytes =
        (const unsigned char *)
        source->data +
        diagnostic->start_offset;

    offset = 0u;

    while (offset < length) {
        size_t width;
        unsigned char character;

        character = bytes[offset];

        /*
         * Help text is deliberately single-line and terminal-safe.
         */
        if (character < 0x20u ||
            character == 0x7fu ||
            character == '`') {

            return false;
        }

        if (!vitte_help_utf8_sequence_valid(
                bytes + offset,
                length - offset,
                &width
            )) {

            return false;
        }

        offset += width;
    }

    memcpy(
        text,
        source->data +
            diagnostic->start_offset,
        length
    );

    text[length] = '\0';

    return true;
}

/* ========================================================================= */
/* Diagnostic detail extraction                                              */
/* ========================================================================= */

static const char *
vitte_help_primary_label(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < diagnostic->label_count &&
         index < VITTE_DIAGNOSTIC_MAX_LABELS;
         index++) {

        const vitte_diagnostic_label_t *label;

        label =
            &diagnostic->labels[index];

        if (label->style ==
                VITTE_DIAGNOSTIC_LABEL_PRIMARY &&
            vitte_help_text_present(
                label->message
            )) {

            return label->message;
        }
    }

    return NULL;
}

static const char *
vitte_help_first_cause(
    const vitte_diagnostic_t *diagnostic
)
{
    size_t index;

    if (diagnostic == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < diagnostic->cause_count &&
         index < VITTE_DIAGNOSTIC_MAX_CAUSES;
         index++) {

        if (vitte_help_text_present(
                diagnostic->causes[index].message
            )) {

            return diagnostic->causes[index].message;
        }
    }

    return NULL;
}

static const char *
vitte_help_detail(
    const vitte_diagnostic_t *diagnostic
)
{
    const char *label;
    const char *cause;

    if (diagnostic == NULL) {
        return NULL;
    }

    if (vitte_help_text_present(
            diagnostic->details
        )) {

        return diagnostic->details;
    }

    label =
        vitte_help_primary_label(
            diagnostic
        );

    if (vitte_help_text_present(label)) {
        return label;
    }

    cause =
        vitte_help_first_cause(
            diagnostic
        );

    if (vitte_help_text_present(cause)) {
        return cause;
    }

    return NULL;
}

/* ========================================================================= */
/* Expected-token extraction                                                 */
/* ========================================================================= */

static bool
vitte_help_extract_expected_token(
    const char *reason,
    char *expected,
    size_t expected_capacity,
    char *found,
    size_t found_capacity
)
{
    const char *start;
    const char *separator;
    const char *end;
    size_t length;

    if (expected != NULL &&
        expected_capacity != 0u) {
        expected[0] = '\0';
    }

    if (found != NULL &&
        found_capacity != 0u) {
        found[0] = '\0';
    }

    if (!vitte_help_text_present(reason) ||
        expected == NULL ||
        expected_capacity == 0u) {
        return false;
    }

    start = reason;

    while (*start != '\0' &&
           isspace((unsigned char)*start)) {
        start++;
    }

    if (strncmp(
            start,
            "expected ",
            9u
        ) != 0) {

        return false;
    }

    start += 9u;

    separator =
        strstr(
            start,
            ", found "
        );

    end =
        separator != NULL
        ? separator
        : start + strlen(start);

    while (end > start &&
           isspace(
               (unsigned char)end[-1]
           )) {

        end--;
    }

    length =
        (size_t)(end - start);

    if (length == 0u) {
        return false;
    }

    if (length >= expected_capacity) {
        length =
            expected_capacity - 1u;
    }

    memcpy(
        expected,
        start,
        length
    );

    expected[length] = '\0';

    if (separator != NULL &&
        found != NULL &&
        found_capacity != 0u) {

        const char *found_start;
        const char *found_end;

        found_start =
            separator + 8u;

        while (*found_start != '\0' &&
               isspace(
                   (unsigned char)*found_start
               )) {

            found_start++;
        }

        found_end =
            found_start +
            strlen(found_start);

        while (found_end > found_start &&
               isspace(
                   (unsigned char)found_end[-1]
               )) {

            found_end--;
        }

        length =
            (size_t)(found_end -
                     found_start);

        if (length >= found_capacity) {
            length =
                found_capacity - 1u;
        }

        memcpy(
            found,
            found_start,
            length
        );

        found[length] = '\0';
    }

    return true;
}

/* ========================================================================= */
/* Specialized remediation                                                   */
/* ========================================================================= */

static bool
vitte_help_format_type_mismatch(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *source_text
)
{
    if (builder == NULL ||
        diagnostic == NULL ||
        !vitte_help_text_present(
            diagnostic->expected_type
        ) ||
        !vitte_help_text_present(
            diagnostic->actual_type
        )) {

        return false;
    }

    vitte_help_builder_append(
        builder,
        "Provide a value of type "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        diagnostic->expected_type,
        VITTE_HELP_TYPE_LIMIT
    );

    vitte_help_builder_append(
        builder,
        " instead of "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        diagnostic->actual_type,
        VITTE_HELP_TYPE_LIMIT
    );

    if (vitte_help_text_present(
            source_text
        )) {

        vitte_help_builder_append(
            builder,
            " for "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            source_text,
            VITTE_HELP_SUBJECT_LIMIT
        );
    }

    vitte_help_builder_append(
        builder,
        ". "
    );

    if (vitte_help_string_type(
            diagnostic->actual_type
        ) &&
        vitte_help_numeric_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "If the value is text input, parse it and handle failure before using it; if text is intended, change the consumer or its declared type instead."
        );

        return true;
    }

    if (vitte_help_numeric_type(
            diagnostic->actual_type
        ) &&
        vitte_help_string_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "If text is intended, format the number explicitly; if the consumer needs a number, correct its declared type instead of converting the value to text."
        );

        return true;
    }

    if (vitte_help_integer_type(
            diagnostic->actual_type
        ) &&
        vitte_help_integer_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "If the destination type is intended, check range and signedness before an explicit conversion; otherwise widen or correct the destination type to preserve the value."
        );

        return true;
    }

    if (vitte_help_float_type(
            diagnostic->actual_type
        ) &&
        vitte_help_integer_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "If an integer is intended, verify the value is integral and in range before converting; if the fraction matters, use a floating-point destination instead."
        );

        return true;
    }

    if (vitte_help_integer_type(
            diagnostic->actual_type
        ) &&
        vitte_help_float_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "If the floating-point destination is intended, convert explicitly after checking precision; if exact integer precision matters, keep an integer destination instead."
        );

        return true;
    }

    if (vitte_help_boolean_type(
            diagnostic->expected_type
        )) {

        vitte_help_builder_append(
            builder,
            "Produce a boolean expression explicitly, normally with a comparison or another operation whose result type is `bool`."
        );

        return true;
    }

    vitte_help_builder_append(
        builder,
        "If the required type is intentional, produce a value of that type; if the actual type is intentional, correct the declaration or operation that requires a different type. Convert only when the conversion preserves the intended value."
    );

    return true;
}

static bool
vitte_help_format_parser_expectation(
    vitte_help_builder_t *builder,
    const char *reason
)
{
    char expected[
        VITTE_HELP_EXPECTED_TOKEN_LIMIT
    ];

    char found[
        VITTE_HELP_EXPECTED_TOKEN_LIMIT
    ];

    if (builder == NULL ||
        !vitte_help_extract_expected_token(
            reason,
            expected,
            sizeof(expected),
            found,
            sizeof(found)
        )) {

        return false;
    }

    vitte_help_builder_append(
        builder,
        "Insert or restore "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        expected,
        VITTE_HELP_EXPECTED_TOKEN_LIMIT
    );

    if (vitte_help_text_present(found)) {
        vitte_help_builder_append(
            builder,
            " before "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            found,
            VITTE_HELP_EXPECTED_TOKEN_LIMIT
        );
    }

    vitte_help_builder_append(
        builder,
        ". Match the expected element to the surrounding grammar and the nearest corresponding opening construct."
    );

    return true;
}

static bool
vitte_help_format_arity_mismatch(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic
)
{
    if (builder == NULL || diagnostic == NULL ||
        !vitte_help_text_present(diagnostic->expected) ||
        !vitte_help_text_present(diagnostic->found)) {
        return false;
    }

    vitte_help_builder_append(builder, "This call supplies ");
    vitte_help_builder_append_quoted_fragment(
        builder, diagnostic->found, VITTE_HELP_EXPECTED_TOKEN_LIMIT
    );
    vitte_help_builder_append(builder, ", while the selected signature requires ");
    vitte_help_builder_append_quoted_fragment(
        builder, diagnostic->expected, VITTE_HELP_EXPECTED_TOKEN_LIMIT
    );
    vitte_help_builder_append(
        builder,
        ". If the call is wrong, add or remove arguments to match the declaration; if the API is wrong, change its parameter list instead."
    );
    return true;
}

static bool
vitte_help_format_unresolved_name(
    vitte_help_builder_t *builder,
    const char *subject
)
{
    if (builder == NULL ||
        !vitte_help_text_present(subject)) {
        return false;
    }

    vitte_help_builder_append(
        builder,
        "Declare or import "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        subject,
        VITTE_HELP_SUBJECT_LIMIT
    );

    vitte_help_builder_append(
        builder,
        " in this scope, or correct its spelling to match a visible declaration."
    );

    return true;
}

static bool
vitte_help_format_unknown_type(
    vitte_help_builder_t *builder,
    const char *subject
)
{
    if (builder == NULL ||
        !vitte_help_text_present(subject)) {
        return false;
    }

    vitte_help_builder_append(
        builder,
        "Declare or import the type "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        subject,
        VITTE_HELP_SUBJECT_LIMIT
    );

    vitte_help_builder_append(
        builder,
        " before using it here; verify the type's spelling, module and visibility."
    );

    return true;
}

static bool
vitte_help_format_boolean_condition(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic
)
{
    if (builder == NULL ||
        diagnostic == NULL ||
        !vitte_help_text_present(
            diagnostic->actual_type
        )) {

        return false;
    }

    vitte_help_builder_append(
        builder,
        "Convert the condition into an explicit boolean expression instead of using a value of type "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        diagnostic->actual_type,
        VITTE_HELP_TYPE_LIMIT
    );

    vitte_help_builder_append(
        builder,
        ". Choose a comparison or predicate that expresses the intended condition."
    );

    return true;
}

static bool
vitte_help_format_contract(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *policy
)
{
    if (builder == NULL ||
        diagnostic == NULL ||
        !diagnostic->has_contract ||
        !vitte_help_text_present(
            diagnostic->contract.expression
        )) {

        return false;
    }

    if (vitte_help_text_present(policy)) {
        vitte_help_builder_append(
            builder,
            policy
        );

        vitte_help_builder_append(
            builder,
            " "
        );
    }

    vitte_help_builder_append(
        builder,
        "Contract condition: "
    );

    vitte_help_builder_append_quoted_fragment(
        builder,
        diagnostic->contract.expression,
        VITTE_HELP_CONTRACT_LIMIT
    );

    vitte_help_builder_append(
        builder,
        "."
    );

    if (vitte_help_text_present(
            diagnostic->contract.reason
        )) {

        vitte_help_builder_append(
            builder,
            " Reported reason: "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->contract.reason,
            VITTE_HELP_REASON_LIMIT
        );

        vitte_help_builder_append(
            builder,
            "."
        );
    }

    return true;
}

/* ========================================================================= */
/* Context enrichment                                                        */
/* ========================================================================= */

static bool vitte_help_code_has_prefix(
    const char *code,
    const char *prefix
);

static bool
vitte_help_wants_context(
    const char *code
)
{
    return (code != NULL &&
            (strcmp(code, "E0300") == 0 ||
             strcmp(code, "E0301") == 0 ||
             strcmp(code, "E0302") == 0 ||
             strcmp(code, "E0303") == 0 ||
             strcmp(code, "E0304") == 0 ||
             strcmp(code, "E0305") == 0 ||
             strcmp(code, "E0306") == 0 ||
             strcmp(code, "E0307") == 0)) ||
        vitte_help_code_has_prefix(code, "E04") ||
        vitte_help_code_has_prefix(code, "E05") ||
        vitte_help_code_has_prefix(code, "E06") ||
        vitte_help_code_has_prefix(code, "E07") ||
        vitte_help_code_has_prefix(code, "E08") ||
        vitte_help_code_has_prefix(code, "E09") ||
        vitte_help_code_has_prefix(code, "E10") ||
        vitte_help_code_has_prefix(code, "E12") ||
        vitte_help_code_has_prefix(code, "E19");
}

static bool
vitte_help_wants_semantic_context(
    const char *code
)
{
    return vitte_help_code_has_prefix(code, "E04") ||
        vitte_help_code_has_prefix(code, "E05") ||
        vitte_help_code_has_prefix(code, "E06") ||
        vitte_help_code_has_prefix(code, "E07") ||
        vitte_help_code_has_prefix(code, "E08") ||
        vitte_help_code_has_prefix(code, "E12");
}

static bool
vitte_help_wants_related_context(
    const char *code
)
{
    return (code != NULL &&
            (strcmp(code, "E0300") == 0 ||
             strcmp(code, "E0301") == 0 ||
             strcmp(code, "E0302") == 0 ||
             strcmp(code, "E0303") == 0 ||
             strcmp(code, "E0304") == 0 ||
             strcmp(code, "E0305") == 0 ||
             strcmp(code, "E0306") == 0 ||
             strcmp(code, "E0307") == 0)) ||
        vitte_help_code_has_prefix(code, "E04") ||
        vitte_help_code_has_prefix(code, "E05") ||
        vitte_help_code_has_prefix(code, "E06") ||
        vitte_help_code_has_prefix(code, "E07") ||
        vitte_help_code_has_prefix(code, "E08") ||
        vitte_help_code_has_prefix(code, "E09") ||
        vitte_help_code_has_prefix(code, "E10") ||
        vitte_help_code_has_prefix(code, "E12") ||
        vitte_help_code_has_prefix(code, "E19");
}

static bool
vitte_help_builder_contains(
    const vitte_help_builder_t *builder,
    const char *text
)
{
    return builder != NULL && builder->buffer != NULL &&
        vitte_help_text_present(text) &&
        strstr(builder->buffer, text) != NULL;
}

static bool
vitte_help_subject_from_message(
    const char *message,
    const char *marker,
    char *subject,
    size_t capacity
)
{
    const char *begin;
    const char *end;
    size_t length;

    if (!vitte_help_text_present(message) ||
        !vitte_help_text_present(marker) ||
        subject == NULL || capacity == 0u) {
        return false;
    }

    subject[0] = '\0';
    begin = strstr(message, marker);
    if (begin == NULL) {
        return false;
    }

    begin += strlen(marker);
    end = strchr(begin, '`');
    if (end == NULL || end == begin) {
        return false;
    }

    length = (size_t)(end - begin);
    if (length >= capacity) {
        return false;
    }

    memcpy(subject, begin, length);
    subject[length] = '\0';
    return true;
}

static void
vitte_help_append_semantic_context(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    bool include_types,
    bool include_symbol,
    bool include_expected_found
)
{
    if (builder == NULL ||
        diagnostic == NULL) {
        return;
    }

    if (include_symbol && vitte_help_text_present(
            diagnostic->symbol
        ) && !vitte_help_builder_contains(builder, diagnostic->symbol)) {

        vitte_help_builder_append(
            builder,
            " Concerned symbol: "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->symbol,
            VITTE_HELP_SUBJECT_LIMIT
        );

        vitte_help_builder_append(
            builder,
            "."
        );
    }

    if (include_types) {
        if (vitte_help_text_present(
                diagnostic->expected_type
            ) && !vitte_help_builder_contains(
                builder, diagnostic->expected_type
            )) {

            vitte_help_builder_append(
                builder,
                " Required type: "
            );

            vitte_help_builder_append_quoted_fragment(
                builder,
                diagnostic->expected_type,
                VITTE_HELP_TYPE_LIMIT
            );

            vitte_help_builder_append(
                builder,
                "."
            );
        }

        if (vitte_help_text_present(
                diagnostic->actual_type
            ) && !vitte_help_builder_contains(
                builder, diagnostic->actual_type
            )) {

            vitte_help_builder_append(
                builder,
                " Actual type: "
            );

            vitte_help_builder_append_quoted_fragment(
                builder,
                diagnostic->actual_type,
                VITTE_HELP_TYPE_LIMIT
            );

            vitte_help_builder_append(
                builder,
                "."
            );
        }
    }

    if (include_expected_found &&
        vitte_help_text_present(diagnostic->expected) &&
        (!vitte_help_text_present(diagnostic->expected_type) ||
         strcmp(diagnostic->expected, diagnostic->expected_type) != 0) &&
        !vitte_help_builder_contains(builder, diagnostic->expected)) {
        vitte_help_builder_append(builder, " Expected: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->expected,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
    }

    if (include_expected_found &&
        vitte_help_text_present(diagnostic->found) &&
        (!vitte_help_text_present(diagnostic->actual_type) ||
         strcmp(diagnostic->found, diagnostic->actual_type) != 0) &&
        !vitte_help_builder_contains(builder, diagnostic->found)) {
        vitte_help_builder_append(builder, " Found: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->found,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
    }
}

static void
vitte_help_append_context_details(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *source_text,
    const char *reason,
    bool include_source,
    const char *public_code
)
{
    const char *label;
    const char *cause;
    size_t index;
    size_t shown;
    bool cause_appended;

    if (builder == NULL || diagnostic == NULL ||
        !vitte_help_wants_context(public_code)) {
        return;
    }

    if (include_source && vitte_help_text_present(source_text)) {
        if (!vitte_help_builder_contains(builder, source_text)) {
            vitte_help_builder_append(builder, " Source excerpt: ");
            vitte_help_builder_append_quoted_fragment(
                builder,
                source_text,
                VITTE_HELP_SUBJECT_LIMIT
            );
            vitte_help_builder_append(builder, ".");
        }
    }

    label = vitte_help_primary_label(diagnostic);
    if (vitte_help_text_present(label) &&
        (!vitte_help_text_present(reason) || strcmp(label, reason) != 0) &&
        !vitte_help_builder_contains(builder, label)) {
        vitte_help_builder_append(builder, " Offending construct: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            label,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
    }

    cause_appended = false;
    if (vitte_help_text_present(diagnostic->probable_cause) &&
        (!vitte_help_text_present(reason) ||
         strcmp(diagnostic->probable_cause, reason) != 0) &&
        !vitte_help_builder_contains(builder, diagnostic->probable_cause)) {
        vitte_help_builder_append(builder, " Likely cause: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->probable_cause,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
        cause_appended = true;
    }

    if (vitte_help_text_present(diagnostic->context) &&
        strcmp(diagnostic->context, "use declaration") != 0 &&
        !vitte_help_builder_contains(builder, diagnostic->context)) {
        vitte_help_builder_append(builder, " Diagnostic context: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->context,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
    }

    cause = vitte_help_first_cause(diagnostic);
    if (vitte_help_text_present(cause) &&
        !cause_appended &&
        (!vitte_help_text_present(reason) || strcmp(cause, reason) != 0) &&
        (!vitte_help_text_present(diagnostic->probable_cause) ||
         strcmp(cause, diagnostic->probable_cause) != 0) &&
        !vitte_help_builder_contains(builder, cause)) {
        vitte_help_builder_append(builder, " Failure cause: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            cause,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
    }

    if (!vitte_help_wants_related_context(public_code)) {
        return;
    }

    shown = 0u;
    for (index = 0u;
         index < diagnostic->related_count &&
         index < VITTE_DIAGNOSTIC_MAX_RELATED &&
         shown < 2u;
         index++) {
        const char *related_label = diagnostic->related[index].label;
        if (!vitte_help_text_present(related_label) ||
            vitte_help_builder_contains(builder, related_label)) {
            continue;
        }
        vitte_help_builder_append(builder, " Related location: ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            related_label,
            VITTE_HELP_REASON_LIMIT
        );
        vitte_help_builder_append(builder, ".");
        shown++;
    }
}

static void
vitte_help_append_compilation_context(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *public_code
)
{
    bool show_module;
    bool show_procedure;

    if (builder == NULL ||
        diagnostic == NULL ||
        !vitte_help_wants_context(public_code)) {
        return;
    }

    show_module = !diagnostic->has_span &&
        ((public_code != NULL &&
            (strcmp(public_code, "E0300") == 0 ||
             strcmp(public_code, "E0301") == 0 ||
             strcmp(public_code, "E0302") == 0 ||
             strcmp(public_code, "E0303") == 0 ||
             strcmp(public_code, "E0304") == 0 ||
             strcmp(public_code, "E0305") == 0 ||
             strcmp(public_code, "E0306") == 0 ||
             strcmp(public_code, "E0307") == 0)) ||
         vitte_help_code_has_prefix(public_code, "E04") ||
         vitte_help_code_has_prefix(public_code, "E05"));
    show_procedure = !diagnostic->has_span &&
        (vitte_help_code_has_prefix(public_code, "E06") ||
        vitte_help_code_has_prefix(public_code, "E07") ||
        vitte_help_code_has_prefix(public_code, "E08") ||
        vitte_help_code_has_prefix(public_code, "E12"));

    if (show_module && vitte_help_text_present(diagnostic->package) &&
        !vitte_help_builder_contains(builder, diagnostic->package)) {
        vitte_help_builder_append(builder, " Module context: package ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->package,
            VITTE_DIAGNOSTIC_PACKAGE_CAPACITY
        );
        vitte_help_builder_append(builder, ".");
    }

    if (show_module && vitte_help_text_present(diagnostic->module) &&
        !vitte_help_builder_contains(builder, diagnostic->module)) {
        vitte_help_builder_append(builder, " Module context: module ");
        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->module,
            VITTE_DIAGNOSTIC_MODULE_CAPACITY
        );
        vitte_help_builder_append(builder, ".");
    }

    if (show_procedure && vitte_help_text_present(diagnostic->procedure) &&
        !vitte_help_builder_contains(builder, diagnostic->procedure)) {
        vitte_help_builder_append(builder, " Procedure context: ");

        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->procedure,
            VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
        );

        vitte_help_builder_append(builder, ".");
    } else if (vitte_help_text_present(diagnostic->contract.procedure)) {
        if (show_procedure && !vitte_help_builder_contains(
                builder, diagnostic->contract.procedure
            )) {
            vitte_help_builder_append(builder, " Procedure context: ");
            vitte_help_builder_append_quoted_fragment(
                builder,
                diagnostic->contract.procedure,
                VITTE_DIAGNOSTIC_PROCEDURE_CAPACITY
            );
            vitte_help_builder_append(builder, ".");
        }
    }
}

static bool
vitte_help_code_has_prefix(
    const char *code,
    const char *prefix
)
{
    return code != NULL && prefix != NULL &&
        strncmp(code, prefix, strlen(prefix)) == 0;
}

static void
vitte_help_append_next_action(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *public_code
)
{
    bool registered;
    bool internal;
    bool backend;

    if (builder == NULL || diagnostic == NULL) {
        return;
    }

    registered = vitte_help_text_present(public_code) &&
        vitte_diagnostic_registry_find_public(public_code) != NULL;
    if (!registered) {
        if (!vitte_help_builder_contains(builder, "Preserve the original command")) {
            vitte_help_builder_append(
                builder,
                " Preserve the original command, compiler version, source and phase for manual investigation."
            );
        }
        return;
    }

    if (vitte_help_builder_contains(builder, "vitte ") ||
        vitte_help_builder_contains(builder, "VITTE_BACKTRACE=1") ||
        vitte_help_builder_contains(builder, "cc -dumpmachine")) {
        return;
    }

    internal = diagnostic->is_internal ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_INTERNAL;
    backend = vitte_help_code_has_prefix(public_code, "E09") ||
        vitte_help_code_has_prefix(public_code, "E10") ||
        vitte_help_code_has_prefix(public_code, "E19") ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_LINKER ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_CODEGEN ||
        diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_IR;

    if (vitte_help_text_present(public_code) &&
        strcmp(public_code, "E0004") == 0) {
        vitte_help_builder_append(
            builder,
            " Next: check memory and process limits, then retry with `vitte compile <file> --jobs=1` if parallel compilation may be exhausting resources."
        );
    } else if (vitte_help_text_present(public_code) &&
               strcmp(public_code, "E0013") == 0) {
        vitte_help_builder_append(
            builder,
            " Next: identify the unavailable component in the reported cause and install or configure it; preserve the reproducer if it is compiler-owned."
        );
    } else if (internal) {
        vitte_help_builder_append(
            builder,
            " Next: rerun the original command with `VITTE_BACKTRACE=1` and preserve the smallest reproducing input."
        );
    } else if (vitte_help_text_present(public_code) &&
               strcmp(public_code, "E0011") == 0) {
        vitte_help_builder_append(
            builder,
            " Next: run `vitte --help` to check the supported options and their combinations."
        );
    } else if (vitte_help_code_has_prefix(public_code, "E19")) {
        vitte_help_builder_append(
            builder,
            " Next: inspect the selected compiler's target with `cc -dumpmachine` when supported (or use the executable named by `CC`), then reproduce with `vitte compile <file>`."
        );
    } else if (backend) {
        vitte_help_builder_append(
            builder,
            " Next: reproduce with `vitte compile <file>`; for C17 backend output, inspect `vitte compile <file> --emit-c`."
        );
    } else if (diagnostic->origin == VITTE_DIAGNOSTIC_ORIGIN_RUNTIME) {
        vitte_help_builder_append(
            builder,
            " Next: rerun `vitte run <file>` with the same input and compare the reported procedure and values."
        );
    } else {
        /* Registered source diagnostics already carry family-specific advice. */
        return;
    }
}

/* ========================================================================= */
/* Fallback help                                                             */
/* ========================================================================= */

static void
vitte_help_format_fallback(
    vitte_help_builder_t *builder,
    const vitte_diagnostic_t *diagnostic,
    const char *reason
)
{
    const char *origin_name;
    const char *identifier;

    if (builder == NULL ||
        diagnostic == NULL) {
        return;
    }

    identifier =
        vitte_help_text_present(
            diagnostic->short_code
        )
        ? diagnostic->short_code
        : diagnostic->code;

    origin_name =
        vitte_diagnostic_origin_name(
            diagnostic->origin
        );

    vitte_help_builder_append(
        builder,
        "No verified correction is registered for "
    );

    if (vitte_help_text_present(identifier)) {
        vitte_help_builder_append_quoted_fragment(
            builder,
            identifier,
            VITTE_DIAGNOSTIC_CODE_CAPACITY
        );
    } else {
        vitte_help_builder_append(
            builder,
            "this unclassified diagnostic"
        );
    }

    if (vitte_help_text_present(origin_name)) {
        vitte_help_builder_append(
            builder,
            " from compiler phase "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            origin_name,
            64u
        );
    }

    vitte_help_builder_append(
        builder,
        "."
    );

    if (vitte_help_text_present(
            diagnostic->message
        )) {

        vitte_help_builder_append(
            builder,
            " Failure: "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            diagnostic->message,
            VITTE_HELP_REASON_LIMIT
        );

        vitte_help_builder_append(
            builder,
            "."
        );
    }

    if (vitte_help_text_present(reason)) {
        vitte_help_builder_append(
            builder,
            " Detail: "
        );

        vitte_help_builder_append_quoted_fragment(
            builder,
            reason,
            VITTE_HELP_REASON_LIMIT
        );

        vitte_help_builder_append(
            builder,
            "."
        );
    }

    if (vitte_help_text_present(
            vitte_help_first_cause(
                diagnostic
            )
        )) {

        const char *cause;

        cause =
            vitte_help_first_cause(
                diagnostic
            );

        if (cause != reason) {
            vitte_help_builder_append(
                builder,
                " Investigate the reported cause: "
            );

            vitte_help_builder_append_quoted_fragment(
                builder,
                cause,
                VITTE_HELP_REASON_LIMIT
            );

            vitte_help_builder_append(
                builder,
                "."
            );
        }
    }

    if (diagnostic->origin ==
            VITTE_DIAGNOSTIC_ORIGIN_INTERNAL ||
        diagnostic->severity ==
            VITTE_DIAGNOSTIC_FATAL) {

        vitte_help_builder_append(
            builder,
            " Preserve the failing command, compiler version and smallest reproducing input for a compiler report."
        );
    }
}

/* ========================================================================= */
/* Contextual help formatter                                                 */
/* ========================================================================= */

bool
vitte_diagnostic_format_help(
    const vitte_diagnostic_t *diagnostic,
    const vitte_source_context_t *sources,
    char *buffer,
    size_t capacity
)
{
    vitte_help_builder_t builder;

    const vitte_diagnostic_registry_entry_t *entry;

    const char *explicit_help;
    const char *public_code;
    const char *policy;
    const char *reason;
    const char *subject;
    char parser_reason[
        (VITTE_HELP_EXPECTED_TOKEN_LIMIT * 2u) + 32u
    ];

    char source_text[
        VITTE_HELP_EXCERPT_CAPACITY
    ];
    char message_subject[
        VITTE_HELP_SUBJECT_LIMIT
    ];

    bool specialized;
    bool include_types;
    bool include_symbol;
    bool include_expected_found;
    bool include_source;

    if (buffer == NULL ||
        capacity == 0u) {
        return false;
    }

    vitte_help_builder_init(
        &builder,
        buffer,
        capacity
    );

    if (diagnostic == NULL) {
        return false;
    }

    /*
     * Explicit source-specific help always wins.
     *
     * A producer that knows the exact remediation has more information than
     * the generic code policy.
     */
    explicit_help =
        vitte_help_explicit_annotation(
            diagnostic
        );

    if (vitte_help_text_present(
            explicit_help
        )) {

        vitte_help_builder_append(
            &builder,
            explicit_help
        );

        if (diagnostic->severity == VITTE_DIAGNOSTIC_WARNING ||
            diagnostic->severity == VITTE_DIAGNOSTIC_ERROR ||
            diagnostic->severity == VITTE_DIAGNOSTIC_FATAL) {
            entry = vitte_help_registry_entry(diagnostic);
            public_code = vitte_help_public_code(diagnostic, entry);
            source_text[0] = '\0';
            (void)vitte_help_source_excerpt(
                diagnostic,
                sources,
                source_text,
                sizeof(source_text)
            );
            if (vitte_help_wants_semantic_context(public_code)) {
                vitte_help_append_semantic_context(
                    &builder, diagnostic, true, true, true
                );
            }
            vitte_help_append_context_details(
                &builder, diagnostic, source_text,
                vitte_help_detail(diagnostic), true, public_code
            );
            vitte_help_append_compilation_context(
                &builder, diagnostic, public_code
            );
        }

        return !builder.failed &&
               builder.length != 0u;
    }

    /*
     * NOTE and HELP diagnostics should not recursively generate generic help.
     *
     * Warnings, errors and fatals may.
     */
    if (diagnostic->severity !=
            VITTE_DIAGNOSTIC_WARNING &&
        diagnostic->severity !=
            VITTE_DIAGNOSTIC_ERROR &&
        diagnostic->severity !=
            VITTE_DIAGNOSTIC_FATAL) {

        return false;
    }

    entry =
        vitte_help_registry_entry(
            diagnostic
        );

    public_code =
        vitte_help_public_code(
            diagnostic,
            entry
        );

    policy =
        vitte_diagnostic_help_for_code(
            public_code
        );

    reason =
        vitte_help_detail(
            diagnostic
        );

    /*
     * Native frontend diagnostics carry expected/found as structured fields
     * and may not include the bootstrap parser's prose detail. Reconstruct
     * the stable parser phrase so contextual help remains equally specific.
     */
    if (public_code != NULL &&
        (strcmp(public_code, "E0201") == 0 ||
         strcmp(public_code, "E0207") == 0 ||
         strcmp(public_code, "E0208") == 0) &&
        vitte_help_text_present(diagnostic->expected)) {
        if (vitte_help_text_present(diagnostic->found)) {
            (void)snprintf(
                parser_reason,
                sizeof(parser_reason),
                "expected %s, found %s",
                diagnostic->expected,
                diagnostic->found
            );
        } else {
            (void)snprintf(
                parser_reason,
                sizeof(parser_reason),
                "expected %s",
                diagnostic->expected
            );
        }
        reason = parser_reason;
    }

    source_text[0] = '\0';

    (void)vitte_help_source_excerpt(
        diagnostic,
        sources,
        source_text,
        sizeof(source_text)
    );

    subject =
        vitte_help_text_present(
            diagnostic->symbol
        )
        ? diagnostic->symbol
        : (
            vitte_help_text_present(
                source_text
            )
            ? source_text
            : NULL
        );

    message_subject[0] = '\0';
    if (subject == NULL && public_code != NULL &&
        strcmp(public_code, "E0401") == 0 &&
        vitte_help_subject_from_message(
            diagnostic->message,
            "unknown symbol `",
            message_subject,
            sizeof(message_subject)
        )) {
        subject = message_subject;
    } else if (subject == NULL && public_code != NULL &&
               strcmp(public_code, "E0510") == 0 &&
               vitte_help_subject_from_message(
                   diagnostic->message,
                   "unknown type `",
                   message_subject,
                   sizeof(message_subject)
               )) {
        subject = message_subject;
    }

    specialized = false;
    include_types = true;
    include_symbol = true;
    include_expected_found = true;
    include_source = true;

    /* --------------------------------------------------------------------- */
    /* Type mismatch / argument / return diagnostics                         */
    /* --------------------------------------------------------------------- */

    if (public_code != NULL &&
        (
            strcmp(public_code, "E0501") == 0 ||
            strcmp(public_code, "E0602") == 0 ||
            strcmp(public_code, "E0605") == 0
        )) {

        specialized =
            vitte_help_format_type_mismatch(
                &builder,
                diagnostic,
                source_text
            );

        if (specialized) {
            include_types = false;
            include_source = false;
        }
    }

    if (!specialized && public_code != NULL &&
        strcmp(public_code, "E0601") == 0) {
        specialized = vitte_help_format_arity_mismatch(
            &builder,
            diagnostic
        );
        if (specialized) {
            include_expected_found = false;
        }
    }

    /* --------------------------------------------------------------------- */
    /* Parser expectation diagnostics                                        */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        public_code != NULL &&
        (
            strcmp(public_code, "E0201") == 0 ||
            strcmp(public_code, "E0207") == 0 ||
            strcmp(public_code, "E0208") == 0
        )) {

        specialized =
            vitte_help_format_parser_expectation(
                &builder,
                reason
            );
        if (specialized) {
            include_expected_found = false;
        }
    }

    /* --------------------------------------------------------------------- */
    /* Name resolution                                                       */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        public_code != NULL &&
        strcmp(public_code, "E0401") == 0) {
        if (vitte_help_text_present(reason) &&
            (entry == NULL || entry->explanation == NULL ||
             strcmp(reason, entry->explanation) != 0)) {
            vitte_help_builder_append(&builder, reason);
            specialized = true;
            include_symbol =
                !vitte_help_text_present(diagnostic->symbol) ||
                strstr(reason, diagnostic->symbol) == NULL;
        } else {
            specialized =
                vitte_help_format_unresolved_name(
                    &builder,
                    subject
                );
            if (specialized) {
                include_symbol = false;
                if (subject != NULL && source_text[0] != '\0' &&
                    strcmp(subject, source_text) == 0) {
                    include_source = false;
                }
            }
        }
    }

    /* --------------------------------------------------------------------- */
    /* Unknown type                                                          */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        public_code != NULL &&
        strcmp(public_code, "E0510") == 0) {

        specialized =
            vitte_help_format_unknown_type(
                &builder,
                subject
            );
        if (specialized) {
            include_symbol = false;
            if (subject != NULL && source_text[0] != '\0' &&
                strcmp(subject, source_text) == 0) {
                include_source = false;
            }
        }
    }

    /* --------------------------------------------------------------------- */
    /* Boolean condition                                                     */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        public_code != NULL &&
        strcmp(public_code, "E0515") == 0) {

        specialized =
            vitte_help_format_boolean_condition(
                &builder,
                diagnostic
            );

        if (specialized) {
            include_types = false;
        }
    }

    /* --------------------------------------------------------------------- */
    /* Contract diagnostics                                                  */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        diagnostic->has_contract) {

        specialized =
            vitte_help_format_contract(
                &builder,
                diagnostic,
                policy
            );
    }

    /* --------------------------------------------------------------------- */
    /* Generic registered remediation                                        */
    /* --------------------------------------------------------------------- */

    if (!specialized &&
        vitte_help_text_present(
            policy
        )) {

        vitte_help_builder_append(
            &builder,
            policy
        );

        specialized = true;
    }

    /* --------------------------------------------------------------------- */
    /* Unknown diagnostic fallback                                           */
    /* --------------------------------------------------------------------- */

    if (!specialized) {
        if (entry != NULL &&
            entry->origin != VITTE_DIAGNOSTIC_ORIGIN_INTERNAL) {
            /* Registered user-facing diagnostics keep their own explanation
             * if a policy-table regression is introduced. */
            vitte_help_builder_append(
                &builder,
                entry->explanation
            );
        } else {
            vitte_help_format_fallback(
                &builder,
                diagnostic,
                reason
            );
        }
    } else {
        /*
         * Structured information augments generic help only when it has not
         * already been incorporated by a specialized formatter.
         */
        if (public_code != NULL &&
            (
                strcmp(public_code, "E0005") == 0 ||
                strcmp(public_code, "E0011") == 0 ||
                strcmp(public_code, "E0013") == 0
            ) &&
            vitte_help_text_present(reason)) {

            vitte_help_builder_append(
                &builder,
                " Reported failure: "
            );

            vitte_help_builder_append_quoted_fragment(
                &builder,
                reason,
                VITTE_HELP_REASON_LIMIT
            );

            vitte_help_builder_append(
                &builder,
                "."
            );
        }

    }

    if (vitte_help_wants_semantic_context(public_code)) {
        vitte_help_append_semantic_context(
            &builder,
            diagnostic,
            include_types,
            include_symbol,
            include_expected_found
        );
    }

    vitte_help_append_context_details(
        &builder,
        diagnostic,
        source_text,
        reason,
        include_source,
        public_code
    );

    vitte_help_append_compilation_context(
        &builder,
        diagnostic,
        public_code
    );

    vitte_help_append_next_action(
        &builder,
        diagnostic,
        public_code
    );

    return !builder.failed &&
           builder.length != 0u;
}

/* ========================================================================= */
/* Rule-table introspection                                                   */
/* ========================================================================= */

size_t
vitte_diagnostic_help_rule_count(void)
{
    return VITTE_HELP_RULE_COUNT;
}

const char *
vitte_diagnostic_help_rule_code(
    size_t index
)
{
    if (index >=
        VITTE_HELP_RULE_COUNT) {
        return NULL;
    }

    return vitte_help_rules[index].code;
}

const char *
vitte_diagnostic_help_rule_text(
    size_t index
)
{
    if (index >=
        VITTE_HELP_RULE_COUNT) {
        return NULL;
    }

    return vitte_help_rules[index].help;
}

/* ========================================================================= */
/* Coverage / validation                                                     */
/* ========================================================================= */

bool
vitte_diagnostic_help_validate_rules(
    size_t *missing_count,
    size_t *orphan_count,
    size_t *duplicate_count
)
{
    size_t missing;
    size_t orphan;
    size_t duplicate;

    size_t registry_index;
    size_t rule_index;

    size_t registry_count;

    missing = 0u;
    orphan = 0u;
    duplicate = 0u;

    registry_count =
        vitte_diagnostic_registry_count();

    /*
     * Every registered warning/error/fatal diagnostic must have a help rule.
     *
     * NOTE/HELP registry entries are informational by nature and are not
     * required to recursively produce remediation guidance.
     */
    for (registry_index = 0u;
         registry_index < registry_count;
         registry_index++) {

        const vitte_diagnostic_registry_entry_t *entry;
        bool found;

        entry =
            vitte_diagnostic_registry_at(
                registry_index
            );

        if (entry == NULL ||
            !vitte_help_text_present(
                entry->public_code
            )) {

            continue;
        }

        if (entry->default_severity ==
                VITTE_DIAGNOSTIC_NOTE ||
            entry->default_severity ==
                VITTE_DIAGNOSTIC_HELP) {

            continue;
        }

        found = false;

        for (rule_index = 0u;
             rule_index < VITTE_HELP_RULE_COUNT;
             rule_index++) {

            if (strcmp(
                    entry->public_code,
                    vitte_help_rules[
                        rule_index
                    ].code
                ) == 0) {

                found = vitte_help_text_present(
                    vitte_help_rules[rule_index].help
                );
                break;
            }
        }

        if (!found) {
            missing++;
        }
    }

    /*
     * Every rule must refer to an existing registry diagnostic.
     */
    for (rule_index = 0u;
         rule_index < VITTE_HELP_RULE_COUNT;
         rule_index++) {

        if (vitte_diagnostic_registry_find_public(
                vitte_help_rules[
                    rule_index
                ].code
            ) == NULL) {

            orphan++;
        }
    }

    /*
     * Duplicate help policies would make behavior depend on table order.
     */
    for (rule_index = 0u;
         rule_index < VITTE_HELP_RULE_COUNT;
         rule_index++) {

        size_t other;

        for (other = rule_index + 1u;
             other < VITTE_HELP_RULE_COUNT;
             other++) {

            if (strcmp(
                    vitte_help_rules[
                        rule_index
                    ].code,
                    vitte_help_rules[
                        other
                    ].code
                ) == 0) {

                duplicate++;
            }
        }
    }

    if (missing_count != NULL) {
        *missing_count = missing;
    }

    if (orphan_count != NULL) {
        *orphan_count = orphan;
    }

    if (duplicate_count != NULL) {
        *duplicate_count = duplicate;
    }

    return missing == 0u &&
           orphan == 0u &&
           duplicate == 0u;
}
