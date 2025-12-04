# LVM Bytecode Specification

This document defines the **logical format** and **execution model** of the Vitte LVM bytecode as produced by the compiler and consumed by the runtime/VM.

The specification is versioned and intended as a _source of truth_ for:

- the code generator,
- the loader (`chunk` / `loader`),
- the disassembler (`disasm`),
- tools (inspectors, optimisers, debuggers).

It deliberately separates:

1. **Logical structure** (constants, functions, instructions, semantics).
2. **Binary encoding** (on‑disk / in‑memory representation).

The exact Rust/Vitte struct names may diverge, but the semantics must remain compatible.

---

## 1. Design goals

The LVM bytecode is designed to be:

- **Stable**: small, well‑defined changes; explicit versioning.
- **Portable**: platform‑independent endianness and sizes.
- **Tool‑friendly**: easy to scan, traverse, and disassemble.
- **Incremental‑friendly**: functions and constants can be added without rewriting the entire file.
- **Reasonably compact** while still being readable in a disassembly.

---

## 2. Conventions

- All integers are **little‑endian**.
- Sizes:
  - `u8`  – 8‑bit unsigned.
  - `u16` – 16‑bit unsigned.
  - `u32` – 32‑bit unsigned.
  - `i32` – 32‑bit signed.
  - `u64` – 64‑bit unsigned.
  - `f64` – 64‑bit IEEE‑754 float.
- Offsets and sizes are always measured in **bytes** from the start of the file.
- Indexes (`const_index`, `func_index`, `local_index`) are zero‑based unless stated otherwise.
- A **chunk** is the compilation unit the VM loads at once; a file encodes exactly one chunk.

---

## 3. Overall file layout

A bytecode file (`.lvm` or `.vbc`) has the following top‑level layout:

```text
+----------------------+  0
| FileHeader           |
+----------------------+  fixed size
| SectionDirectory     |  variable (N entries)
+----------------------+  ...
| ConstantPool         |
+----------------------+
| FunctionTable        |
+----------------------+
| CodeSection          |
+----------------------+
| DebugSection (opt.)  |
+----------------------+  EOF
```

Each section is referenced by the directory using:

- a **section kind** tag,
- a **byte offset**,
- a **byte length**.

This allows tools to skip sections they do not need (e.g. debug).

---

## 4. File header

### 4.1. Binary layout

```text
struct FileHeader {
    u32 magic;          // 'LVM0' in ASCII: 0x30_4D_56_4C (little‑endian)
    u16 version_major;  // bytecode format major version
    u16 version_minor;  // bytecode format minor version
    u32 flags;          // reserved for future use
    u32 reserved_0;     // must be zero
    u32 reserved_1;     // must be zero
    u32 section_count;  // number of entries in the SectionDirectory
}
```

- `magic`:

  - must be `0x304D564C` in little‑endian (ASCII `"LVM0"`).
  - loaders MUST reject files with an unexpected magic.

- `version_major` / `version_minor`:

  - define the **on‑disk** format version.
  - breaking changes increment `version_major`.
  - additive / backward‑compatible changes increment `version_minor`.

- `flags`:

  - bitfield reserved for compression, encryption, or other toggles.
  - current spec: all bits MUST be zero; loaders SHOULD reject unknown bits.

---

## 5. Section directory

Immediately after `FileHeader` comes an array of `section_count` entries:

```text
struct SectionEntry {
    u16 kind;       // SectionKind
    u16 flags;      // section-specific flags (compression, etc.)
    u32 offset;     // byte offset from start of file
    u32 length;     // byte length of the section
}
```

Known `SectionKind` values:

| Kind | Name            | Description                        |
|------|-----------------|------------------------------------|
| 0    | Reserved        | (unused)                           |
| 1    | ConstantPool    | Tagged constants                   |
| 2    | FunctionTable   | Function prototypes                |
| 3    | CodeSection     | Raw instruction stream             |
| 4    | DebugSection    | Optional debug information         |

Loaders MAY ignore unknown section kinds as long as they do not rely on them.

---

## 6. Constant pool

The **constant pool** is a sequential list of tagged constants. Each constant has:

```text
struct ConstantHeader {
    u8  tag;    // ConstantTag
    u8  pad0;   // reserved, must be 0
    u16 pad1;   // reserved, must be 0
}
```

Followed by a tag‑specific payload.

### 6.1. Tag space

| Tag | Name            | Payload                                  |
|-----|-----------------|-------------------------------------------|
| 0   | ConstNil        | no payload                               |
| 1   | ConstBool       | `u8 value` (0 = false, 1 = true)         |
| 2   | ConstI64        | `i64 value`                              |
| 3   | ConstF64        | `f64 value`                              |
| 4   | ConstString     | `u32 len` + `len` bytes UTF‑8            |
| 5   | ConstFunction   | `u32 func_index`                         |
| 6   | ConstBytes      | `u32 len` + `len` bytes arbitrary        |
| 7   | ConstReserved   | reserved for future use                  |

The logical constant index is the ordinal position within the pool: first constant = index 0.

---

## 7. Function table

The function table describes **callable units**. Bytecode instructions reference functions by their index.

```text
struct FunctionEntry {
    u32 name_const;     // index into ConstantPool for the function's name (ConstString)
    u32 code_offset;    // byte offset into CodeSection where instructions start
    u32 code_size;      // byte size of this function's instruction stream
    u16 param_count;    // number of positional parameters
    u16 local_count;    // number of local slots (excluding parameters)
    u16 max_stack;      // maximum stack depth for this function
    u16 flags;          // FunctionFlags (see below)
}
```

### 7.1. Function flags

Bitfield:

- `0x0001` – `FFLAG_EXPORT` : function is exported from the chunk.
- `0x0002` – `FFLAG_VARARGS`: function accepts a variadic list after `param_count`.

Other bits are reserved and MUST be zero for now.

The **entrypoint** of a chunk is defined as:

- the first function with `FFLAG_EXPORT` and name `"main"`, OR
- function index `0` if no export is flagged (tool‑specific).

---

## 8. Execution model

The VM is a **stack‑based** machine with the following state:

- **Operand stack**: holds temporary values.
- **Call stack**: sequence of frames.
- Each **frame** contains:
  - `locals[]`: slots for parameters and locals (`local_count` total).
  - `pc`: program counter (byte offset into `CodeSection`).
  - `func_index`: index in `FunctionTable`.

Instructions operate on:

- the **current frame**,
- the **operand stack**,
- global state (globals table, constant pool, etc.).

Errors (stack underflow, invalid jump target, bad constant index) are **VM faults** and abort execution unless handled by a higher layer.

---

## 9. Instruction encoding

### 9.1. General layout

Each instruction is encoded as one or more 32‑bit words.

- The **first word** always uses the same structure:

```text
bits  0..7   : opcode (u8)
bits  8..15  : A (u8)
bits 16..23  : B (u8)
bits 24..31  : C (u8)
```

- For many instructions, `A/B/C` are used directly as 8‑bit fields (register index, small constants, small offsets).
- Instructions that require a **32‑bit immediate** value use an additional word:

```text
word0: opcode / A / B / C
word1: imm32 (u32)  // interpretation depends on opcode
```

Disassemblers SHOULD render both as a single logical instruction.

### 9.2. Addressing conventions

- `const_index` is typically a `u32` (immediate word).
- `local_index` is typically an 8‑bit value (fits in `A`, `B`, or `C`).
- Relative jump offsets are signed 32‑bit, measured in **words** forward/backward from the next instruction.

---

## 10. Opcode set (v1)

This is the logical opcode set for version `1.x` of the bytecode. The numeric values are stable within a major version.

### 10.1. Stack and control

| Code | Mnemonic       | Form                          | Stack effect           | Description                                      |
|------|----------------|-------------------------------|------------------------|--------------------------------------------------|
| 0x00 | `NOP`          | —                             | `… -> …`               | No operation.                                    |
| 0x01 | `HALT`         | —                             | `… -> (halt)`          | Stop execution of the whole VM.                 |
| 0x02 | `POP`          | —                             | `…, v -> …`            | Pop top of stack and discard.                   |
| 0x03 | `DUP`          | —                             | `…, v -> …, v, v`      | Duplicate top of stack.                         |

### 10.2. Constants and loading/storing

| Code | Mnemonic       | Form                              | Stack effect                | Description                                           |
|------|----------------|-----------------------------------|-----------------------------|-------------------------------------------------------|
| 0x10 | `CONST`        | `A = unused, imm32 = const_idx`   | `… -> …, c`                 | Push `const_pool[const_idx]` onto stack.             |
| 0x11 | `LOAD_LOCAL`   | `A = local_index`                 | `… -> …, v`                 | Push `locals[local_index]`.                          |
| 0x12 | `STORE_LOCAL`  | `A = local_index`                 | `…, v -> …`                 | Pop and store into `locals[local_index]`.            |
| 0x13 | `LOAD_GLOBAL`  | `imm32 = global_index`            | `… -> …, v`                 | Push global variable value.                          |
| 0x14 | `STORE_GLOBAL` | `imm32 = global_index`            | `…, v -> …`                 | Pop and store into global variable.                  |

### 10.3. Unary and binary operators

All binary operations follow the pattern:

```text
…, a, b -> …, result
```

where `b` is the top of stack.

| Code | Mnemonic       | Stack effect          | Description                                 |
|------|----------------|-----------------------|---------------------------------------------|
| 0x20 | `NEG`          | `…, v -> …, (-v)`     | Arithmetic negation.                        |
| 0x21 | `NOT`          | `…, v -> …, !v`       | Boolean negation.                           |
| 0x22 | `ADD`          | `…, a, b -> …, a+b`   | Addition.                                   |
| 0x23 | `SUB`          | `…, a, b -> …, a-b`   | Subtraction.                                |
| 0x24 | `MUL`          | `…, a, b -> …, a*b`   | Multiplication.                             |
| 0x25 | `DIV`          | `…, a, b -> …, a/b`   | Division (VM‑defined semantics for integer).|
| 0x26 | `MOD`          | `…, a, b -> …, a%b`   | Remainder.                                  |

### 10.4. Comparisons

Comparisons push a boolean.

| Code | Mnemonic       | Stack effect              | Description                        |
|------|----------------|---------------------------|------------------------------------|
| 0x30 | `EQ`           | `…, a, b -> …, (a==b)`    | Equal.                             |
| 0x31 | `NE`           | `…, a, b -> …, (a!=b)`    | Not equal.                         |
| 0x32 | `LT`           | `…, a, b -> …, (a<b)`     | Less than.                         |
| 0x33 | `LE`           | `…, a, b -> …, (a<=b)`    | Less or equal.                     |
| 0x34 | `GT`           | `…, a, b -> …, (a>b)`     | Greater than.                      |
| 0x35 | `GE`           | `…, a, b -> …, (a>=b)`    | Greater or equal.                  |

### 10.5. Branching

All jumps use a 32‑bit signed offset (`imm32`) measured in words from the instruction _after_ the jump.

| Code | Mnemonic          | Form                         | Stack effect        | Description                                      |
|------|-------------------|------------------------------|---------------------|--------------------------------------------------|
| 0x40 | `JUMP`            | `imm32 = rel_offset`         | `… -> …`            | Unconditional jump.                              |
| 0x41 | `JUMP_IF_TRUE`    | `imm32 = rel_offset`         | `…, cond -> …`      | Jump if `cond` is truthy.                        |
| 0x42 | `JUMP_IF_FALSE`   | `imm32 = rel_offset`         | `…, cond -> …`      | Jump if `cond` is falsy.                         |

---

## 11. Calls and returns

### 11.1. Call opcodes

| Code | Mnemonic       | Form                       | Stack effect                          | Description                                        |
|------|----------------|----------------------------|---------------------------------------|----------------------------------------------------|
| 0x50 | `CALL`         | `A = argc`                 | `…, f, arg₀..argₙ₋₁ -> …, result`     | Call function `f` with `argc` args.                |
| 0x51 | `TAILCALL`     | `A = argc`                 | `…, f, arg₀..argₙ₋₁ -> …, result`     | Tail call, reusing current frame.                 |
| 0x52 | `RET`          | `A = unused`               | `…, result -> (caller stack), result` | Return from current function with one result.     |

Details:

- The callee is expected to be a function value referring to an entry in the function table; if not, it is a VM fault.
- `CALL` pushes a new frame; `TAILCALL` replaces the current frame.

---

## 12. Example encoding

Example: a function that returns constant `42`:

```text
FunctionEntry:
    name_const   = index("main")
    code_offset  = X
    code_size    = Y
    param_count  = 0
    local_count  = 0
    max_stack    = 1
    flags        = FFLAG_EXPORT

CodeSection at offset X:

  0: CONST k      ; push const[k] where const[k] = 42
  2: RET          ; return top of stack

Binary (words):

  word0 = 0x10 00 00 00   ; opcode CONST (0x10), A/B/C unused
  word1 = 0x0000000A      ; const index k = 10 (example)

  word2 = 0x52 00 00 00   ; opcode RET (0x52)
```

A disassembler would render:

```text
0000: CONST   #10
0002: RET
```

---

## 13. Versioning & compatibility

- This document describes **bytecode format v1.x**.
- Tools MUST check `version_major` and either:
  - support it explicitly; or
  - reject with a clear error.

Minor version changes (`version_minor`) may introduce:

- new opcodes at the end of the opcode table,
- new section kinds (tools may ignore unknown sections),
- new flags, as long as they are interpreted conservatively.

---

## 14. Open extensions

Areas explicitly left open for future revisions:

- Compressed code sections (section flags).
- Additional constant tags (e.g. `ConstStruct`, `ConstArray`).
- Structured exception handling opcodes.
- Generator/coroutine instructions.

Any such change MUST be reflected both:

- in this specification, and
- in the corresponding `bytecode` modules (chunk, ops, disasm, loader).

This spec is the authoritative reference for how chunks are serialized and executed by the LVM.
