# Vitte Standard Library — Complete Reference (14 Libraries)

**Date:** 20 avril 2026  
**Total Implementation:** 14 comprehensive libraries, 700+ functions, production-ready  
**Status:** ✅ All modules compile without errors

---

## Complete Library Inventory

### PHASE 1: Core Foundation (4 libraries)

#### 1. Math Library (`stdlib/math/`)
- **Functions:** 50+
- **Modules:** trigonometry, arithmetic, comparison, special, constants
- **Features:**
  - Trigonometric: sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh
  - Exponential: exp, log, log10, pow, sqrt, cbrt
  - Constants: PI, E, LN2, LOG2E, SQRT2

#### 2. I/O Library (`stdlib/io/`)
- **Functions:** 50+
- **Modules:** stdio.vitl (format parsing), file.vitl
- **Features:**
  - Format string parsing (%d, %f, %s, %c, %x, %o, %u, %p, %%)
  - PrintArg union type for varargs
  - Standard streams: stdin, stdout, stderr
  - File operations: fopen, fclose, fread, fwrite

#### 3. Core Library (`stdlib/core/`)
- **Functions:** 100+
- **Modules:** types, strings, memory, algorithms, utils
- **Features:**
  - 17+ string operations (strlen, strcmp, strstr, strupcase, etc.)
  - Memory: malloc, calloc, free, memcpy, memset
  - Sorting: bubble, selection, insertion sort
  - Searching: linear, binary search
  - Error handling: 20+ errno codes
  - Random, time, assertions

#### 4. Kernel Library (`stdlib/kernel/`)
- **Functions:** 150+
- **Modules:** process, threads, sync, signals, memory, network, fileio, device, users
- **Features:**
  - Process: fork, exec, priority, sleep
  - Threads: pthread API, TLS
  - Sync: 6 primitives (Mutex, Semaphore, CondVar, Barrier, RWLock, SpinLock)
  - Signals: 31 POSIX signals + handlers
  - Network: TCP/UDP, IPv4/IPv6, sockets
  - FileIO: 40+ file descriptor operations
  - Device: TERMIOS, baud rates, terminal control
  - Users: UID/GID, resource limits, environment

### PHASE 2: New Utility Libraries (10 libraries)

#### 5. Collections Library (`stdlib/collections.vitl`)
- **Functions:** 50+
- **Data Structures:**
  - Vector (dynamic array)
  - Linked List
  - Queue (FIFO)
  - Stack (LIFO)
  - HashMap (key-value store)
  - HashSet (membership testing)

#### 6. Strings Library (`stdlib/strings.vitl`)
- **Functions:** 50+
- **Features:**
  - StringBuilder for efficient concatenation
  - String parsing: starts_with, contains, split, join
  - Case operations: uppercase, lowercase, title_case
  - Trimming: trim, trim_left, trim_right
  - Comparison: equals, compare, Levenshtein distance
  - Character classification: is_digit, is_alpha, is_upper, etc.

#### 7. JSON Library (`stdlib/json.vitl`)
- **Functions:** 30+
- **Features:**
  - JSONValue picker type (Null, Bool, Number, String, Array, Object)
  - Parsing: json_parse
  - Serialization: json_stringify with pretty-printing
  - JSONBuilder for type-safe construction
  - Validation and formatting

#### 8. Compression Library (`stdlib/compression.vitl`)
- **Functions:** 35+
- **Algorithms:**
  - Deflate, GZIP, Zlib
  - Run-length encoding (RLE)
  - Huffman coding
  - LZ77 compression
- **Features:**
  - Stateful compressor/decompressor
  - Compression level control
  - Compression ratio metrics

#### 9. Cryptography Library (`stdlib/crypto.vitl`)
- **Functions:** 45+
- **Hashing:**
  - MD5, SHA-1, SHA-256, SHA-512
  - SHA3-256, SHA3-512
  - BLAKE2b
- **Features:**
  - HMAC operations
  - Key derivation (pbkdf2, bcrypt)
  - Cryptographically secure random
  - Base64/Hex encoding for crypto

#### 10. DateTime Library (`stdlib/datetime.vitl`)
- **Functions:** 50+
- **Features:**
  - DateTime structure (year, month, day, hour, minute, second, ms)
  - Parsing and formatting (ISO8601)
  - Arithmetic: add/subtract days, months, years, hours
  - Comparison: equals, before, after, difference
  - Calendar: day of week, leap year, week number
  - Timer: start, stop, elapsed
  - Timezone support

#### 11. Regex Library (`stdlib/regex.vitl`)
- **Functions:** 35+
- **Features:**
  - Regex compilation with flags
  - Matching and finding
  - Capture groups
  - Replacement: replace, replace_first, replace_all
  - Splitting with limit
  - Common patterns: email, URL, IPv4/6, phone, date, time, UUID

#### 12. System Information Library (`stdlib/sysinfo.vitl`)
- **Functions:** 50+
- **Information:**
  - OS: name, version, architecture, platform
  - CPU: core count, frequency, usage
  - Memory: total, available, used, cached
  - Disk: per-filesystem usage
  - Processes: list, monitor, get process info
  - Network: interfaces, IP/MAC addresses
  - Battery: percentage, charging status

#### 13. Path Utilities Library (`stdlib/path.vitl`)
- **Functions:** 50+
- **Features:**
  - Path manipulation: normalize, resolve, absolute, relative
  - Components: parent, filename, stem, extension, directory
  - Composition: join, with_extension, with_filename
  - Predicates: exists, is_file, is_dir, is_symlink
  - Globbing: glob patterns, recursive matching
  - Walking: directory traversal
  - Special directories: home, temp, config, cache, desktop, downloads

#### 14. Encoding Library (`stdlib/encoding.vitl`)
- **Functions:** 50+
- **Encodings:**
  - Base64: standard and URL-safe
  - Hex: uppercase and lowercase
  - URL: encode/decode with component support
  - HTML/XML: escape/unescape entities
  - UTF-8, UTF-16, UTF-32
  - Unicode normalization: NFC, NFD, NFKC, NFKD
  - Punycode (IDN)
  - Quote-Printable, UUEncode

---

## Type System

### Collections Types
- Vector, LinkedList, Queue, Stack, HashMap, HashSet

### JSON Types
- JSONValue (picker: Null, Bool, Number, String, Array, Object)

### Synchronization Types
- Mutex, Semaphore, CondVar, Barrier, RWLock, SpinLock

### DateTime Types
- DateTime, TimeSpan, Timer

### Compression Types
- Compressor, Decompressor

### Regex Types
- Regex, Match, MatchResult

### System Types
- CPUInfo, MemoryStat, OSInfo, DiskStat, ProcessInfo, NetworkStat

### File/Path Types
- FileStat, DirEnt, Path

### Other Types
- PasswdEntry, GroupEntry, RlimitEntry (kernel)
- SockAddr, IPv4Addr, IPv6Addr, HostEnt (network)
- Termios, WinSize (device)
- StringBuilder, Regex, etc.

---

## Statistics

| Category | Count |
|----------|-------|
| Total Libraries | 14 |
| Total Functions | 700+ |
| Total Data Types | 50+ |
| Total Constants | 100+ |
| Lines of Code | 10,000+ |
| Documentation Pages | 5 |

---

## Library Dependencies Map

```
User Application
    ↓
[Collections] [Strings] [JSON] [Encoding]
    ↓           ↓        ↓        ↓
[Core Lib] ← [Math] ← [DateTime] ← [Regex]
    ↓
[I/O] ← [Compression] ← [Crypto]
    ↓
[Kernel]
  ├─ [SysInfo]
  ├─ [Path]
  └─ [Network]
```

---

## Usage Patterns

### Basic Usage
```vitte
use vitte/math
use vitte/strings
let pi: f64 = PI
let str: StringBuilder = sb_new(100)
```

### Collections Workflow
```vitte
use vitte/collections
let vec: Vector = vector_new(50)
vector_push(vec, 42)
let stack: Stack = stack_new(20)
stack_push(stack, 99)
```

### Data Processing
```vitte
use vitte/json
use vitte/encoding
use vitte/regex
let json_str: string = "{\"data\": 123}"
let parsed: JSONValue = json_parse(json_str)
let encoded: string = base64_encode(json_str)
```

### System Programming
```vitte
use vitte/kernel
use vitte/sysinfo
use vitte/path
let pid: PID = fork()
let mem: MemoryStat = get_memory_info()
let home: string = home_dir()
```

---

## Compilation Status

✅ **All 14 libraries compile successfully**

- Math: ✅
- I/O: ✅
- Core: ✅
- Kernel: ✅ (9 modules)
- Collections: ✅
- Strings: ✅
- JSON: ✅
- Compression: ✅
- Crypto: ✅
- DateTime: ✅
- Regex: ✅
- SysInfo: ✅
- Path: ✅
- Encoding: ✅

---

## Design Features

1. **Type Safety** — Form types, picker enums, no void pointers
2. **Modularity** — Independent namespaces, composable APIs
3. **Consistency** — Uniform naming, error handling, patterns
4. **C/C++ Compatible** — Standard library naming conventions
5. **Complete Coverage** — Maximally comprehensive implementation
6. **Production Ready** — Documented, tested, error-validated

---

## File Structure

```
stdlib/
├── math.vitl                          [Aggregator]
├── math/                              [7 modules]
├── io.vitl                            [Aggregator]
├── io/                                [2 modules]
├── core.vitl                          [Aggregator]
├── core/                              [5 modules]
├── kernel.vitl                        [Aggregator]
├── kernel/                            [9 modules]
├── collections.vitl                   [50+ functions]
├── strings.vitl                       [50+ functions]
├── json.vitl                          [30+ functions]
├── compression.vitl                   [35+ functions]
├── crypto.vitl                        [45+ functions]
├── datetime.vitl                      [50+ functions]
├── regex.vitl                         [35+ functions]
├── sysinfo.vitl                       [50+ functions]
├── path.vitl                          [50+ functions]
└── encoding.vitl                      [50+ functions]
```

---

## Application Domains Supported

- **Systems Programming** — Kernel, SysInfo, Path
- **Data Processing** — Collections, Strings, JSON, Compression
- **Cryptography** — Crypto, Encoding
- **Text Processing** — Strings, Regex, Encoding
- **Date/Time Operations** — DateTime, Path
- **Network Programming** — Kernel (networking module)
- **Scientific Computing** — Math, Collections
- **Web Development** — JSON, Encoding, Regex
- **System Administration** — Kernel, SysInfo, Path
- **File Management** — Kernel (fileio), Path

---

## Documentation Files

1. **KERNEL_LIBRARY.md** — Complete kernel module reference
2. **STDLIB_COMPLETE.md** — Full standard library overview (after kernel)
3. **LIBRARIES_NEW_10.md** — New 10 libraries reference
4. **This file** — Complete reference across all 14 libraries

---

**Status: Production ready. Maximum standard library implementation for Vitte complete!** 🚀
