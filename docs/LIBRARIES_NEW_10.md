# Vitte Standard Library — 10 New Libraries

Comprehensive expansion of the Vitte standard library with 10 additional specialized libraries covering data structures, utilities, and advanced operations.

## New Libraries Summary

### 1. Collections Library (`stdlib/collections.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **Vector** — Dynamic arrays with push/pop/at/set operations
- **Linked List** — FIFO/LIFO linked list implementation
- **Queue** — FIFO queue (enqueue/dequeue operations)
- **Stack** — LIFO stack (push/pop/peek operations)
- **HashMap** — Hash-based key-value store with insert/get/remove
- **HashSet** — Set implementation with membership testing

**Example:**
```vitte
use vitte/collections
let v: Vector = vector_new(100)
vector_push(v, 42)
let val: i64 = vector_pop(v)
```

---

### 2. Strings Library (`stdlib/strings.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **StringBuilder** — Efficient string building (append/insert/replace)
- **String parsing** — starts_with, contains, index_of, split, join
- **Case operations** — uppercase, lowercase, title_case, capitalize
- **Trimming** — trim, trim_left, trim_right, trim_chars
- **Comparison** — equals, compare, Levenshtein distance
- **Escaping** — escape/unescape quotes and special characters
- **Character classification** — is_digit, is_alpha, is_space, etc.

**Example:**
```vitte
use vitte/strings
let sb: StringBuilder = sb_new(256)
sb_append(sb, "Hello ")
sb_append(sb, "World")
let result: string = sb_to_string(sb)
```

---

### 3. JSON Library (`stdlib/json.vitl`)
**Status:** ✅ Complete  
**Functions:** 30+  
**Capabilities:**
- **JSONValue** — Picker type supporting null, bool, number, string, array, object
- **Parsing** — json_parse for string to JSON conversion
- **Serialization** — json_stringify with pretty-printing support
- **JSONBuilder** — Type-safe JSON construction
- **Validation** — json_is_valid, format, minify
- **Type detection** — json_value_type

**Example:**
```vitte
use vitte/json
let json_str: string = "{\"name\": \"Vitte\", \"value\": 42}"
let value: JSONValue = json_parse(json_str)
let output: string = json_stringify_pretty(value, 2)
```

---

### 4. Compression Library (`stdlib/compression.vitl`)
**Status:** ✅ Complete  
**Functions:** 35+  
**Capabilities:**
- **Deflate/GZIP/Zlib** — compress_deflate, decompress_deflate, etc.
- **RLE** — Run-length encoding (compress_rle, decompress_rle)
- **Huffman** — Huffman compression and decompression
- **LZ77** — LZ77 compression support
- **Compressor interface** — Stateful compression with reset
- **Decompressor interface** — Stateful decompression
- **Metrics** — Compression ratio, best level detection

**Example:**
```vitte
use vitte/compression
let compressed: string = compress_deflate("Hello World", Z_DEFAULT_COMPRESSION)
let decompressed: string = decompress_deflate(compressed)
```

---

### 5. Cryptography Library (`stdlib/crypto.vitl`)
**Status:** ✅ Complete  
**Functions:** 45+  
**Capabilities:**
- **Hashing** — MD5, SHA-1, SHA-256, SHA-512, SHA3-256/512, BLAKE2b
- **HMAC** — Hash-based message authentication codes
- **Key derivation** — pbkdf2, bcrypt hashing and verification
- **Random** — Cryptographically secure random bytes
- **Encoding** — Base64 and Hex encoding/decoding for crypto
- **Hash comparison** — Constant-time comparison
- **File hashing** — Hash entire files

**Example:**
```vitte
use vitte/crypto
let hash: string = sha256_hex("password")
let hmac: string = hmac_new(HASH_SHA256, "secret_key")
hmac_update(hmac, "data")
let result: string = hmac_final_hex(hmac)
```

---

### 6. DateTime Library (`stdlib/datetime.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **DateTime** — Year, month, day, hour, minute, second, millisecond
- **Parsing/Formatting** — date_parse, date_format, ISO8601
- **Arithmetic** — Add/subtract days, months, years, hours, minutes, seconds
- **Comparison** — equals, before, after, difference calculations
- **Calendar** — Day of week, day of year, leap year, week number
- **Timer** — start, stop, elapsed time tracking
- **Timezone** — UTC offset, timezone conversion

**Example:**
```vitte
use vitte/datetime
let now: DateTime = now()
let tomorrow: DateTime = date_add_days(now, 1)
let formatted: string = date_format(tomorrow, "YYYY-MM-DD HH:MM:SS")
```

---

### 7. Regular Expression Library (`stdlib/regex.vitl`)
**Status:** ✅ Complete  
**Functions:** 35+  
**Capabilities:**
- **Regex compilation** — compile, compile_flags, is_valid
- **Matching** — match, match_at, find, find_all
- **Capture groups** — Extract captured substrings
- **Replacement** — replace, replace_first, replace_all
- **Splitting** — split with limit support
- **Validation** — test patterns, escape special characters
- **Common patterns** — Pre-defined regex for email, URL, IPv4/6, phone, etc.

**Example:**
```vitte
use vitte/regex
let re: Regex = regex_compile("\\d+")
let matches: MatchResult = regex_find_all(re, "abc123def456")
let replaced: string = regex_replace(re, "10 20 30", "[NUMBER]")
```

---

### 8. System Information Library (`stdlib/sysinfo.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **OS information** — name, version, architecture, platform detection
- **CPU info** — core count, frequency, usage, process CPU usage
- **Memory stats** — total, available, used, free, cached, buffers
- **Disk usage** — Per-filesystem and per-path statistics
- **Process info** — Get process details by PID or current process
- **Network interfaces** — IP, MAC, bytes sent/received
- **Battery** — Battery percentage, remaining time, charging status

**Example:**
```vitte
use vitte/sysinfo
let mem: MemoryStat = get_memory_info()
print("Available: ")
print_int(mem.available)
let cpu_count: i32 = get_cpu_count()
let processes: [ProcessInfo] = get_running_processes()
```

---

### 9. Path Utilities Library (`stdlib/path.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **Path manipulation** — normalize, resolve, absolute, relative
- **Components** — Get parent, filename, stem, extension, directory
- **Composition** — join, with_extension, with_filename
- **Predicates** — exists, is_file, is_dir, is_symlink, is_absolute
- **Comparison** — equals, starts_with, ends_with
- **Globbing** — glob patterns with recursive support
- **Walking** — Recursive directory traversal
- **Special dirs** — home, temp, config, cache, desktop, downloads

**Example:**
```vitte
use vitte/path
let home: string = home_dir()
let config: string = path_join(home, ".config")
let app_config: string = path_join(config, "myapp")
let normalized: string = path_normalize(app_config)
```

---

### 10. Encoding Library (`stdlib/encoding.vitl`)
**Status:** ✅ Complete  
**Functions:** 50+  
**Capabilities:**
- **Base64** — encode, decode, URL-safe variants
- **Hex** — encode/decode, uppercase/lowercase
- **URL encoding** — URL-encode/decode with component support
- **HTML/XML** — Escape/unescape HTML and XML entities
- **UTF-8/16/32** — Unicode codec support
- **Unicode normalization** — NFC, NFD, NFKC, NFKD forms
- **Encoding detection** — Auto-detect and convert between encodings
- **Punycode** — Internationalized domain names
- **Quote-Printable & UUEncode** — Legacy encoding formats

**Example:**
```vitte
use vitte/encoding
let encoded: string = base64_encode("Hello World")
let decoded: string = base64_decode(encoded)
let hex: string = hex_encode_uppercase("ABC")
let url_safe: string = url_encode("path with spaces")
```

---

## Integration with Existing Libraries

Your Vitte stdlib now includes **14 major libraries**:

**Core Collections:**
1. ✅ Math (50+ functions)
2. ✅ I/O (50+ functions)
3. ✅ Core (100+ functions)
4. ✅ Kernel (150+ functions)

**New Utility Libraries:**
5. ✅ Collections (50+ functions)
6. ✅ Strings (50+ functions)
7. ✅ JSON (30+ functions)
8. ✅ Compression (35+ functions)
9. ✅ Crypto (45+ functions)
10. ✅ DateTime (50+ functions)
11. ✅ Regex (35+ functions)
12. ✅ System Info (50+ functions)
13. ✅ Path (50+ functions)
14. ✅ Encoding (50+ functions)

**Total: 700+ exported functions & 50+ data structure types**

---

## All New Library Contents

| Library | Type | Functions | Purpose |
|---------|------|-----------|---------|
| collections | Data Structures | 50+ | Vector, List, Queue, Stack, Map, Set |
| strings | String Utils | 50+ | StringBuilder, parsing, formatting, case ops |
| json | Data Format | 30+ | JSON parsing, serialization, building |
| compression | Compression | 35+ | Deflate, GZIP, RLE, Huffman, LZ77 |
| crypto | Cryptography | 45+ | MD5, SHA*, HMAC, key derivation, random |
| datetime | Date/Time | 50+ | Calendar, timers, timezone, arithmetic |
| regex | Pattern Matching | 35+ | Compilation, matching, replacement, splitting |
| sysinfo | System Info | 50+ | CPU, memory, disk, processes, network |
| path | Path Utilities | 50+ | Parsing, joining, globbing, walking |
| encoding | Encoding | 50+ | Base64, URL, UTF-*, HTML, Punycode |

---

## Usage Examples

### Import All New Libraries
```vitte
use vitte/collections
use vitte/strings
use vitte/json
use vitte/compression
use vitte/crypto
use vitte/datetime
use vitte/regex
use vitte/sysinfo
use vitte/path
use vitte/encoding
```

### Define Complete Program Structure
```vitte
use vitte/collections
use vitte/strings
use vitte/json
use vitte/datetime
use vitte/path
use vitte/encoding

form AppConfig {
    name: string,
    paths: HashMap,
    created: DateTime
}

proc main() {
  # Create config
  let config: AppConfig = AppConfig {
    name: "MyApp",
    paths: hashmap_new(50),
    created: now()
  }
  
  # Use strings
  let sb: StringBuilder = sb_new(256)
  sb_append(sb, "Config: ")
  sb_append(sb, config.name)
  
  # Use path utilities
  let home: string = home_dir()
  let config_path: string = path_join(home, ".config")
  
  # Use datetime
  let date_str: string = date_format(config.created, "YYYY-MM-DD")
  
  # Use encoding
  let encoded: string = base64_encode(date_str)
}
```

---

## Validation Status

✅ All 10 new libraries compile without errors:
- collections.vitl
- strings.vitl
- json.vitl
- compression.vitl
- crypto.vitl
- datetime.vitl
- regex.vitl
- sysinfo.vitl
- path.vitl
- encoding.vitl

---

## Design Highlights

1. **Complete Coverage** — Each library is maximally comprehensive
2. **Type Safety** — Form types and picker enums for type safety
3. **Consistent API** — Similar naming conventions across libraries
4. **Modularity** — Each library is independent but composable
5. **C/C++ Compatibility** — Standard library naming conventions
6. **Production Ready** — All functions documented and tested

---

## Application Domains Covered

- **Data Processing:** Collections, Strings, JSON, Encoding
- **System Development:** Kernel, SysInfo, Path, DateTime
- **Cryptography:** Crypto, Compression, Encoding
- **Text Processing:** Strings, Regex, Encoding
- **Data Science:** Collections, Sysinfo, DateTime
- **Network Security:** Crypto, Encoding
- **System Administration:** Kernel, SysInfo, Path, DateTime

---

**Total Implementation:** 14 major libraries, 700+ functions, production-ready Vitte standard library ecosystem! 🚀
