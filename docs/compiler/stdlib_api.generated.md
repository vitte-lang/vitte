# Vitte Stdlib API

Generated from `src/vitte/stdlib/stdlib_modules.json`.
Each entry is suitable for LSP symbol indexing and documentation lookup.

## `src/vitte/stdlib/mod.vit`

Stability: `stable`

- `form Vector` signature `form Vector<T> {` example `mod.Vector` stability `stable`
- `form HashMap` signature `form HashMap<K, V> {` example `mod.HashMap` stability `stable`
- `proc vector_new` signature `proc vector_new<T>() -> Vector<T> {` example `mod.vector_new(...)` stability `stable`
- `proc vector_push` signature `proc vector_push<T>(vec: Vector<T>, item: T) -> Vector<T> {` example `mod.vector_push(...)` stability `stable`
- `proc hashmap_new` signature `proc hashmap_new<K, V>() -> HashMap<K, V> {` example `mod.hashmap_new(...)` stability `stable`
- `proc hashmap_insert` signature `proc hashmap_insert<K, V>(map: HashMap<K, V>, key: K, value: V) -> HashMap<K, V> {` example `mod.hashmap_insert(...)` stability `stable`
- `proc io_read_file` signature `proc io_read_file(path: string) -> string {` example `mod.io_read_file(...)` stability `stable`
- `proc io_write_file` signature `proc io_write_file(path: string, content: string) -> bool {` example `mod.io_write_file(...)` stability `stable`
- `proc io_append_file` signature `proc io_append_file(path: string, content: string) -> bool {` example `mod.io_append_file(...)` stability `stable`
- `proc io_file_exists` signature `proc io_file_exists(path: string) -> bool {` example `mod.io_file_exists(...)` stability `stable`
- `proc io_create_directory` signature `proc io_create_directory(path: string) -> bool {` example `mod.io_create_directory(...)` stability `stable`
- `proc io_is_file` signature `proc io_is_file(path: string) -> bool {` example `mod.io_is_file(...)` stability `stable`
- `proc io_is_directory` signature `proc io_is_directory(path: string) -> bool {` example `mod.io_is_directory(...)` stability `stable`
- `proc io_copy_file` signature `proc io_copy_file(src: string, dst: string) -> bool {` example `mod.io_copy_file(...)` stability `stable`
- `proc io_move_file` signature `proc io_move_file(src: string, dst: string) -> bool {` example `mod.io_move_file(...)` stability `stable`
- `proc io_delete_file` signature `proc io_delete_file(path: string) -> bool {` example `mod.io_delete_file(...)` stability `stable`
- `proc io_delete_directory` signature `proc io_delete_directory(path: string) -> bool {` example `mod.io_delete_directory(...)` stability `stable`
- `proc io_list_directory` signature `proc io_list_directory(path: string) -> [string] {` example `mod.io_list_directory(...)` stability `stable`

## `src/vitte/stdlib/core.vitl`

Stability: `stable`

- `const CORE_VERSION` signature `const CORE_VERSION: string = "1.0.0"` example `core.CORE_VERSION` stability `stable`
- `const TRUE` signature `const TRUE: bool = true` example `core.TRUE` stability `stable`
- `const FALSE` signature `const FALSE: bool = false` example `core.FALSE` stability `stable`
- `const OK` signature `const OK: int = 0` example `core.OK` stability `stable`
- `const ERR` signature `const ERR: int = 1` example `core.ERR` stability `stable`
- `const NULL` signature `const NULL: int = 0` example `core.NULL` stability `stable`
- `const I8_MIN` signature `const I8_MIN: i8 = -128` example `core.I8_MIN` stability `stable`
- `const I8_MAX` signature `const I8_MAX: i8 = 127` example `core.I8_MAX` stability `stable`
- `const U8_MAX` signature `const U8_MAX: u8 = 255` example `core.U8_MAX` stability `stable`
- `const I16_MIN` signature `const I16_MIN: i16 = -32768` example `core.I16_MIN` stability `stable`
- `const I16_MAX` signature `const I16_MAX: i16 = 32767` example `core.I16_MAX` stability `stable`
- `const U16_MAX` signature `const U16_MAX: u16 = 65535` example `core.U16_MAX` stability `stable`
- `const I32_MIN` signature `const I32_MIN: i32 = -2147483648` example `core.I32_MIN` stability `stable`
- `const I32_MAX` signature `const I32_MAX: i32 = 2147483647` example `core.I32_MAX` stability `stable`
- `const U32_MAX` signature `const U32_MAX: u32 = 4294967295` example `core.U32_MAX` stability `stable`
- `const I64_MIN` signature `const I64_MIN: i64 = -9223372036854775808` example `core.I64_MIN` stability `stable`
- `const I64_MAX` signature `const I64_MAX: i64 = 9223372036854775807` example `core.I64_MAX` stability `stable`
- `const USIZE_BITS` signature `const USIZE_BITS: int = 64` example `core.USIZE_BITS` stability `stable`
- `const ISIZE_BITS` signature `const ISIZE_BITS: int = 64` example `core.ISIZE_BITS` stability `stable`
- `const F32_EPSILON` signature `const F32_EPSILON: f32 = 0.000001` example `core.F32_EPSILON` stability `stable`
- `const F64_EPSILON` signature `const F64_EPSILON: f64 = 0.000000000001` example `core.F64_EPSILON` stability `stable`
- `const EXIT_SUCCESS` signature `const EXIT_SUCCESS: int = 0` example `core.EXIT_SUCCESS` stability `stable`
- `const EXIT_FAILURE` signature `const EXIT_FAILURE: int = 1` example `core.EXIT_FAILURE` stability `stable`
- `const EPERM` signature `const EPERM: int = 1` example `core.EPERM` stability `stable`
- `const ENOENT` signature `const ENOENT: int = 2` example `core.ENOENT` stability `stable`
- `const ESRCH` signature `const ESRCH: int = 3` example `core.ESRCH` stability `stable`
- `const EINTR` signature `const EINTR: int = 4` example `core.EINTR` stability `stable`
- `const EIO` signature `const EIO: int = 5` example `core.EIO` stability `stable`
- `const ENXIO` signature `const ENXIO: int = 6` example `core.ENXIO` stability `stable`
- `const E2BIG` signature `const E2BIG: int = 7` example `core.E2BIG` stability `stable`
- `const ENOEXEC` signature `const ENOEXEC: int = 8` example `core.ENOEXEC` stability `stable`
- `const EBADF` signature `const EBADF: int = 9` example `core.EBADF` stability `stable`
- `const ECHILD` signature `const ECHILD: int = 10` example `core.ECHILD` stability `stable`
- `const EAGAIN` signature `const EAGAIN: int = 11` example `core.EAGAIN` stability `stable`
- `const ENOMEM` signature `const ENOMEM: int = 12` example `core.ENOMEM` stability `stable`
- `const EACCES` signature `const EACCES: int = 13` example `core.EACCES` stability `stable`
- `const EFAULT` signature `const EFAULT: int = 14` example `core.EFAULT` stability `stable`
- `const EBUSY` signature `const EBUSY: int = 16` example `core.EBUSY` stability `stable`
- `const EEXIST` signature `const EEXIST: int = 17` example `core.EEXIST` stability `stable`
- `const EXDEV` signature `const EXDEV: int = 18` example `core.EXDEV` stability `stable`
- `const ENODEV` signature `const ENODEV: int = 19` example `core.ENODEV` stability `stable`
- `const ENOTDIR` signature `const ENOTDIR: int = 20` example `core.ENOTDIR` stability `stable`
- `const EISDIR` signature `const EISDIR: int = 21` example `core.EISDIR` stability `stable`
- `const EINVAL` signature `const EINVAL: int = 22` example `core.EINVAL` stability `stable`
- `const ENFILE` signature `const ENFILE: int = 23` example `core.ENFILE` stability `stable`
- `const EMFILE` signature `const EMFILE: int = 24` example `core.EMFILE` stability `stable`
- `const ENOTTY` signature `const ENOTTY: int = 25` example `core.ENOTTY` stability `stable`
- `const ETXTBSY` signature `const ETXTBSY: int = 26` example `core.ETXTBSY` stability `stable`
- `const EFBIG` signature `const EFBIG: int = 27` example `core.EFBIG` stability `stable`
- `const ENOSPC` signature `const ENOSPC: int = 28` example `core.ENOSPC` stability `stable`
- `const ESPIPE` signature `const ESPIPE: int = 29` example `core.ESPIPE` stability `stable`
- `const EROFS` signature `const EROFS: int = 30` example `core.EROFS` stability `stable`
- `const EMLINK` signature `const EMLINK: int = 31` example `core.EMLINK` stability `stable`
- `const EPIPE` signature `const EPIPE: int = 32` example `core.EPIPE` stability `stable`
- `const ERANGE` signature `const ERANGE: int = 34` example `core.ERANGE` stability `stable`
- `const ENOSYS` signature `const ENOSYS: int = 38` example `core.ENOSYS` stability `stable`
- `const ENOTEMPTY` signature `const ENOTEMPTY: int = 39` example `core.ENOTEMPTY` stability `stable`
- `const ENOTSUP` signature `const ENOTSUP: int = 95` example `core.ENOTSUP` stability `stable`
- `pick CoreStatus` signature `pick CoreStatus {` example `core.CoreStatus` stability `stable`
- `pick Option` signature `pick Option[T] {` example `core.Option` stability `stable`
- `pick Result` signature `pick Result[T, E] {` example `core.Result` stability `stable`
- `form CoreError` signature `form CoreError {` example `core.CoreError` stability `stable`
- `form Span` signature `form Span {` example `core.Span` stability `stable`
- `form Range` signature `form Range {` example `core.Range` stability `stable`
- `form USizeRange` signature `form USizeRange {` example `core.USizeRange` stability `stable`
- `form Pair` signature `form Pair[A, B] {` example `core.Pair` stability `stable`
- `form Triple` signature `form Triple[A, B, C] {` example `core.Triple` stability `stable`
- `form Slice` signature `form Slice[T] {` example `core.Slice` stability `stable`
- `form Buffer` signature `form Buffer {` example `core.Buffer` stability `stable`
- `form AllocBlock` signature `form AllocBlock {` example `core.AllocBlock` stability `stable`
- `form Version` signature `form Version {` example `core.Version` stability `stable`
- `form CoreManifest` signature `form CoreManifest {` example `core.CoreManifest` stability `stable`
- `form CoreHealth` signature `form CoreHealth {` example `core.CoreHealth` stability `stable`
- `form CoreSummary` signature `form CoreSummary {` example `core.CoreSummary` stability `stable`
- `proc core_version` signature `proc core_version() -> Version {` example `core.core_version(...)` stability `stable`
- `proc core_name` signature `proc core_name() -> string {` example `core.core_name(...)` stability `stable`
- `proc ok` signature `proc ok() -> CoreStatus {` example `core.ok(...)` stability `stable`
- `proc err` signature `proc err() -> CoreStatus {` example `core.err(...)` stability `stable`
- `proc core_error` signature `proc core_error(code: int, message: string) -> CoreError {` example `core.core_error(...)` stability `stable`
- `proc core_modules` signature `proc core_modules() -> [string] {` example `core.core_modules(...)` stability `stable`
- `proc core_module_count` signature `proc core_module_count() -> int {` example `core.core_module_count(...)` stability `stable`
- `proc core_manifest` signature `proc core_manifest() -> CoreManifest {` example `core.core_manifest(...)` stability `stable`
- `proc core_ready` signature `proc core_ready() -> bool {` example `core.core_ready(...)` stability `stable`
- `proc core_health` signature `proc core_health() -> CoreHealth {` example `core.core_health(...)` stability `stable`
- `proc core_summary` signature `proc core_summary() -> CoreSummary {` example `core.core_summary(...)` stability `stable`
- `proc errno_name` signature `proc errno_name(code: int) -> string {` example `core.errno_name(...)` stability `stable`
- `proc strerror` signature `proc strerror(code: int) -> string {` example `core.strerror(...)` stability `stable`
- `proc option_is_some` signature `proc option_is_some[T](value: Option[T]) -> bool {` example `core.option_is_some(...)` stability `stable`
- `proc option_is_none` signature `proc option_is_none[T](value: Option[T]) -> bool {` example `core.option_is_none(...)` stability `stable`
- `proc option_unwrap_or` signature `proc option_unwrap_or[T](value: Option[T], fallback: T) -> T {` example `core.option_unwrap_or(...)` stability `stable`
- `proc result_is_ok` signature `proc result_is_ok[T, E](value: Result[T, E]) -> bool {` example `core.result_is_ok(...)` stability `stable`
- `proc result_is_err` signature `proc result_is_err[T, E](value: Result[T, E]) -> bool {` example `core.result_is_err(...)` stability `stable`
- `proc result_unwrap_or` signature `proc result_unwrap_or[T, E](value: Result[T, E], fallback: T) -> T {` example `core.result_unwrap_or(...)` stability `stable`
- `proc assert_true` signature `proc assert_true(condition: bool, message: string) -> void {` example `core.assert_true(...)` stability `stable`
- `proc assert_false` signature `proc assert_false(condition: bool, message: string) -> void {` example `core.assert_false(...)` stability `stable`
- `proc assert_int_eq` signature `proc assert_int_eq(a: int, b: int, message: string) -> void {` example `core.assert_int_eq(...)` stability `stable`
- `proc assert_int_ne` signature `proc assert_int_ne(a: int, b: int, message: string) -> void {` example `core.assert_int_ne(...)` stability `stable`
- `proc assert_not_null` signature `proc assert_not_null(ptr: usize, message: string) -> void {` example `core.assert_not_null(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `core.min_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `core.max_int(...)` stability `stable`
- `proc clamp_int` signature `proc clamp_int(x: int, low: int, high: int) -> int {` example `core.clamp_int(...)` stability `stable`
- `proc abs_int` signature `proc abs_int(x: int) -> int {` example `core.abs_int(...)` stability `stable`
- `proc sign_int` signature `proc sign_int(x: int) -> int {` example `core.sign_int(...)` stability `stable`
- `proc cmp_int` signature `proc cmp_int(a: int, b: int) -> int {` example `core.cmp_int(...)` stability `stable`
- `proc is_even` signature `proc is_even(x: int) -> bool {` example `core.is_even(...)` stability `stable`
- `proc is_odd` signature `proc is_odd(x: int) -> bool {` example `core.is_odd(...)` stability `stable`
- `proc align_up` signature `proc align_up(value: usize, align: usize) -> usize {` example `core.align_up(...)` stability `stable`
- `proc align_down` signature `proc align_down(value: usize, align: usize) -> usize {` example `core.align_down(...)` stability `stable`
- `proc is_aligned` signature `proc is_aligned(value: usize, align: usize) -> bool {` example `core.is_aligned(...)` stability `stable`
- `proc ptr_null` signature `proc ptr_null() -> usize {` example `core.ptr_null(...)` stability `stable`
- `proc ptr_is_null` signature `proc ptr_is_null(ptr: usize) -> bool {` example `core.ptr_is_null(...)` stability `stable`
- `proc ptr_is_aligned` signature `proc ptr_is_aligned(ptr: usize, align: usize) -> bool {` example `core.ptr_is_aligned(...)` stability `stable`
- `proc ptr_add` signature `proc ptr_add(ptr: usize, offset: usize) -> usize {` example `core.ptr_add(...)` stability `stable`
- `proc ptr_sub` signature `proc ptr_sub(ptr: usize, offset: usize) -> usize {` example `core.ptr_sub(...)` stability `stable`
- `proc ptr_diff` signature `proc ptr_diff(a: usize, b: usize) -> isize {` example `core.ptr_diff(...)` stability `stable`
- `proc bit` signature `proc bit(n: int) -> usize {` example `core.bit(...)` stability `stable`
- `proc bit_is_set` signature `proc bit_is_set(value: usize, n: int) -> bool {` example `core.bit_is_set(...)` stability `stable`
- `proc bit_set` signature `proc bit_set(value: usize, n: int) -> usize {` example `core.bit_set(...)` stability `stable`
- `proc bit_clear` signature `proc bit_clear(value: usize, n: int) -> usize {` example `core.bit_clear(...)` stability `stable`
- `proc bit_toggle` signature `proc bit_toggle(value: usize, n: int) -> usize {` example `core.bit_toggle(...)` stability `stable`
- `proc low_byte` signature `proc low_byte(value: usize) -> u8 {` example `core.low_byte(...)` stability `stable`
- `proc high_byte_u16` signature `proc high_byte_u16(value: u16) -> u8 {` example `core.high_byte_u16(...)` stability `stable`
- `proc make_u16` signature `proc make_u16(lo: u8, hi: u8) -> u16 {` example `core.make_u16(...)` stability `stable`
- `proc swap_u16` signature `proc swap_u16(x: u16) -> u16 {` example `core.swap_u16(...)` stability `stable`
- `proc swap_u32` signature `proc swap_u32(x: u32) -> u32 {` example `core.swap_u32(...)` stability `stable`
- `proc strlen` signature `proc strlen(s: string) -> usize {` example `core.strlen(...)` stability `stable`
- `proc string_is_empty` signature `proc string_is_empty(s: string) -> bool {` example `core.string_is_empty(...)` stability `stable`
- `proc string_eq` signature `proc string_eq(a: string, b: string) -> bool {` example `core.string_eq(...)` stability `stable`
- `proc string_ne` signature `proc string_ne(a: string, b: string) -> bool {` example `core.string_ne(...)` stability `stable`
- `proc string_starts_with` signature `proc string_starts_with(s: string, prefix: string) -> bool {` example `core.string_starts_with(...)` stability `stable`
- `proc string_ends_with` signature `proc string_ends_with(s: string, suffix: string) -> bool {` example `core.string_ends_with(...)` stability `stable`
- `proc string_contains` signature `proc string_contains(s: string, needle: string) -> bool {` example `core.string_contains(...)` stability `stable`
- `proc string_find` signature `proc string_find(s: string, needle: string) -> int {` example `core.string_find(...)` stability `stable`
- `proc string_repeat` signature `proc string_repeat(s: string, count: int) -> string {` example `core.string_repeat(...)` stability `stable`
- `proc string_slice` signature `proc string_slice(s: string, start: int, end: int) -> string {` example `core.string_slice(...)` stability `stable`
- `proc string_reverse` signature `proc string_reverse(s: string) -> string {` example `core.string_reverse(...)` stability `stable`
- `proc string_trim_left` signature `proc string_trim_left(s: string) -> string {` example `core.string_trim_left(...)` stability `stable`
- `proc string_trim_right` signature `proc string_trim_right(s: string) -> string {` example `core.string_trim_right(...)` stability `stable`
- `proc string_trim` signature `proc string_trim(s: string) -> string {` example `core.string_trim(...)` stability `stable`
- `proc char_is_digit` signature `proc char_is_digit(c: char) -> bool {` example `core.char_is_digit(...)` stability `stable`
- `proc char_is_lower` signature `proc char_is_lower(c: char) -> bool {` example `core.char_is_lower(...)` stability `stable`
- `proc char_is_upper` signature `proc char_is_upper(c: char) -> bool {` example `core.char_is_upper(...)` stability `stable`
- `proc char_is_alpha` signature `proc char_is_alpha(c: char) -> bool {` example `core.char_is_alpha(...)` stability `stable`
- `proc char_is_alnum` signature `proc char_is_alnum(c: char) -> bool {` example `core.char_is_alnum(...)` stability `stable`
- `proc char_is_space` signature `proc char_is_space(c: char) -> bool {` example `core.char_is_space(...)` stability `stable`
- `proc char_to_lower` signature `proc char_to_lower(c: char) -> char {` example `core.char_to_lower(...)` stability `stable`
- `proc char_to_upper` signature `proc char_to_upper(c: char) -> char {` example `core.char_to_upper(...)` stability `stable`
- `proc bytes_empty` signature `proc bytes_empty() -> bytes {` example `core.bytes_empty(...)` stability `stable`
- `proc bytes_len` signature `proc bytes_len(b: bytes) -> usize {` example `core.bytes_len(...)` stability `stable`
- `proc bytes_is_empty` signature `proc bytes_is_empty(b: bytes) -> bool {` example `core.bytes_is_empty(...)` stability `stable`
- `proc buffer_new` signature `proc buffer_new(capacity: usize) -> Buffer {` example `core.buffer_new(...)` stability `stable`
- `proc buffer_from_bytes` signature `proc buffer_from_bytes(data: bytes) -> Buffer {` example `core.buffer_from_bytes(...)` stability `stable`
- `proc buffer_clear` signature `proc buffer_clear(buf: Buffer) -> Buffer {` example `core.buffer_clear(...)` stability `stable`
- `proc buffer_is_empty` signature `proc buffer_is_empty(buf: Buffer) -> bool {` example `core.buffer_is_empty(...)` stability `stable`
- `proc slice_new` signature `proc slice_new[T](data: [T]) -> Slice[T] {` example `core.slice_new(...)` stability `stable`
- `proc slice_range` signature `proc slice_range[T](data: [T], start: usize, len: usize) -> Slice[T] {` example `core.slice_range(...)` stability `stable`
- `proc array_len` signature `proc array_len[T](xs: [T]) -> usize {` example `core.array_len(...)` stability `stable`
- `proc array_is_empty` signature `proc array_is_empty[T](xs: [T]) -> bool {` example `core.array_is_empty(...)` stability `stable`
- `proc array_first_int` signature `proc array_first_int(xs: [int]) -> Option[int] {` example `core.array_first_int(...)` stability `stable`
- `proc array_last_int` signature `proc array_last_int(xs: [int]) -> Option[int] {` example `core.array_last_int(...)` stability `stable`
- `proc array_contains_int` signature `proc array_contains_int(xs: [int], value: int) -> bool {` example `core.array_contains_int(...)` stability `stable`
- `proc array_sum_int` signature `proc array_sum_int(xs: [int]) -> int {` example `core.array_sum_int(...)` stability `stable`
- `proc array_min_int` signature `proc array_min_int(xs: [int]) -> Option[int] {` example `core.array_min_int(...)` stability `stable`
- `proc array_max_int` signature `proc array_max_int(xs: [int]) -> Option[int] {` example `core.array_max_int(...)` stability `stable`
- `proc array_reverse_int` signature `proc array_reverse_int(xs: [int]) -> [int] {` example `core.array_reverse_int(...)` stability `stable`
- `proc array_fill_int` signature `proc array_fill_int(count: usize, value: int) -> [int] {` example `core.array_fill_int(...)` stability `stable`
- `proc array_range_int` signature `proc array_range_int(start: int, end: int) -> [int] {` example `core.array_range_int(...)` stability `stable`
- `proc memcmp` signature `proc memcmp(a: bytes, b: bytes, count: usize) -> int {` example `core.memcmp(...)` stability `stable`
- `proc memset` signature `proc memset(buf: bytes, value: u8, count: usize) -> bytes {` example `core.memset(...)` stability `stable`
- `proc memcpy` signature `proc memcpy(dst: bytes, src: bytes, count: usize) -> bytes {` example `core.memcpy(...)` stability `stable`
- `proc memzero` signature `proc memzero(count: usize) -> bytes {` example `core.memzero(...)` stability `stable`
- `proc atoi` signature `proc atoi(s: string) -> i64 {` example `core.atoi(...)` stability `stable`
- `proc parse_i64` signature `proc parse_i64(s: string) -> i64 {` example `core.parse_i64(...)` stability `stable`
- `proc parse_int` signature `proc parse_int(s: string) -> int {` example `core.parse_int(...)` stability `stable`
- `proc parse_bool` signature `proc parse_bool(s: string) -> bool {` example `core.parse_bool(...)` stability `stable`
- `proc itoa` signature `proc itoa(value: i64) -> string {` example `core.itoa(...)` stability `stable`
- `proc to_string_int` signature `proc to_string_int(value: int) -> string {` example `core.to_string_int(...)` stability `stable`
- `proc to_string_i64` signature `proc to_string_i64(value: i64) -> string {` example `core.to_string_i64(...)` stability `stable`
- `proc to_string_bool` signature `proc to_string_bool(value: bool) -> string {` example `core.to_string_bool(...)` stability `stable`
- `proc to_string_status` signature `proc to_string_status(status: CoreStatus) -> string {` example `core.to_string_status(...)` stability `stable`
- `proc hash_int` signature `proc hash_int(value: int) -> usize {` example `core.hash_int(...)` stability `stable`
- `proc hash_string` signature `proc hash_string(s: string) -> usize {` example `core.hash_string(...)` stability `stable`
- `proc range_new` signature `proc range_new(start: int, end: int) -> Range {` example `core.range_new(...)` stability `stable`
- `proc range_len` signature `proc range_len(r: Range) -> int {` example `core.range_len(...)` stability `stable`
- `proc range_contains` signature `proc range_contains(r: Range, value: int) -> bool {` example `core.range_contains(...)` stability `stable`
- `proc span_new` signature `proc span_new(start: usize, end: usize) -> Span {` example `core.span_new(...)` stability `stable`
- `proc span_len` signature `proc span_len(s: Span) -> usize {` example `core.span_len(...)` stability `stable`
- `proc span_contains` signature `proc span_contains(s: Span, value: usize) -> bool {` example `core.span_contains(...)` stability `stable`
- `proc pair` signature `proc pair[A, B](a: A, b: B) -> Pair[A, B] {` example `core.pair(...)` stability `stable`
- `proc triple` signature `proc triple[A, B, C](a: A, b: B, c: C) -> Triple[A, B, C] {` example `core.triple(...)` stability `stable`
- `proc random_seed` signature `proc random_seed(seed: u64) -> u64 {` example `core.random_seed(...)` stability `stable`
- `proc random_next` signature `proc random_next(seed: u64) -> u64 {` example `core.random_next(...)` stability `stable`
- `proc random_range` signature `proc random_range(seed: u64, min: int, max: int) -> int {` example `core.random_range(...)` stability `stable`
- `proc clock_ticks` signature `proc clock_ticks() -> i64 {` example `core.clock_ticks(...)` stability `stable`
- `proc time_seconds` signature `proc time_seconds() -> i64 {` example `core.time_seconds(...)` stability `stable`
- `proc sleep_ms` signature `proc sleep_ms(ms: int) -> int {` example `core.sleep_ms(...)` stability `stable`
- `proc env_get` signature `proc env_get(name: string) -> Option[string] {` example `core.env_get(...)` stability `stable`
- `proc env_has` signature `proc env_has(name: string) -> bool {` example `core.env_has(...)` stability `stable`
- `proc platform_name` signature `proc platform_name() -> string {` example `core.platform_name(...)` stability `stable`
- `proc arch_name` signature `proc arch_name() -> string {` example `core.arch_name(...)` stability `stable`
- `proc is_debug` signature `proc is_debug() -> bool {` example `core.is_debug(...)` stability `stable`
- `proc is_release` signature `proc is_release() -> bool {` example `core.is_release(...)` stability `stable`
- `proc unreachable_core` signature `proc unreachable_core() -> never {` example `core.unreachable_core(...)` stability `stable`
- `proc library_meta` signature `proc library_meta() -> string {` example `core.library_meta(...)` stability `stable`
- `proc core_selftest` signature `proc core_selftest() -> bool {` example `core.core_selftest(...)` stability `stable`

## `src/vitte/stdlib/memory.vitl`

Stability: `stable`

- `const MEMORY_VERSION` signature `const MEMORY_VERSION: string = ""` example `memory.MEMORY_VERSION` stability `stable`
- `const OK` signature `const OK: int = 0` example `memory.OK` stability `stable`
- `const ERR` signature `const ERR: int = 0` example `memory.ERR` stability `stable`
- `const NULL` signature `const NULL: usize = 0` example `memory.NULL` stability `stable`
- `const PAGE_SIZE` signature `const PAGE_SIZE: usize = 0` example `memory.PAGE_SIZE` stability `stable`
- `const HUGE_PAGE_SIZE` signature `const HUGE_PAGE_SIZE: usize = 0` example `memory.HUGE_PAGE_SIZE` stability `stable`
- `const DEFAULT_ALIGN` signature `const DEFAULT_ALIGN: usize = 0` example `memory.DEFAULT_ALIGN` stability `stable`
- `const CACHE_LINE_SIZE` signature `const CACHE_LINE_SIZE: usize = 0` example `memory.CACHE_LINE_SIZE` stability `stable`
- `const KB` signature `const KB: usize = 0` example `memory.KB` stability `stable`
- `const MB` signature `const MB: usize = 0` example `memory.MB` stability `stable`
- `const GB` signature `const GB: usize = 0` example `memory.GB` stability `stable`
- `const MEM_READ` signature `const MEM_READ: int = 0` example `memory.MEM_READ` stability `stable`
- `const MEM_WRITE` signature `const MEM_WRITE: int = 0` example `memory.MEM_WRITE` stability `stable`
- `const MEM_EXEC` signature `const MEM_EXEC: int = 0` example `memory.MEM_EXEC` stability `stable`
- `const MEM_USER` signature `const MEM_USER: int = 0` example `memory.MEM_USER` stability `stable`
- `const MEM_KERNEL` signature `const MEM_KERNEL: int = 0` example `memory.MEM_KERNEL` stability `stable`
- `const MEM_DEVICE` signature `const MEM_DEVICE: int = 0` example `memory.MEM_DEVICE` stability `stable`
- `const MEM_DMA` signature `const MEM_DMA: int = 0` example `memory.MEM_DMA` stability `stable`
- `pick MemoryStatus` signature `pick MemoryStatus {` example `memory.MemoryStatus` stability `stable`
- `pick AllocKind` signature `pick AllocKind {` example `memory.AllocKind` stability `stable`
- `pick PageState` signature `pick PageState {` example `memory.PageState` stability `stable`
- `form MemoryResult` signature `form MemoryResult {` example `memory.MemoryResult` stability `stable`
- `form MemoryBlock` signature `form MemoryBlock {` example `memory.MemoryBlock` stability `stable`
- `form MemoryRegion` signature `form MemoryRegion {` example `memory.MemoryRegion` stability `stable`
- `form Page` signature `form Page {` example `memory.Page` stability `stable`
- `form PageRange` signature `form PageRange {` example `memory.PageRange` stability `stable`
- `form Heap` signature `form Heap {` example `memory.Heap` stability `stable`
- `form Arena` signature `form Arena {` example `memory.Arena` stability `stable`
- `form Pool` signature `form Pool {` example `memory.Pool` stability `stable`
- `form SlabClass` signature `form SlabClass {` example `memory.SlabClass` stability `stable`
- `form SlabAllocator` signature `form SlabAllocator {` example `memory.SlabAllocator` stability `stable`
- `form ByteBuffer` signature `form ByteBuffer {` example `memory.ByteBuffer` stability `stable`
- `form MemoryStats` signature `form MemoryStats {` example `memory.MemoryStats` stability `stable`
- `form MemoryMap` signature `form MemoryMap {` example `memory.MemoryMap` stability `stable`
- `proc memory_ok` signature `proc memory_ok() -> int {` example `memory.memory_ok(...)` stability `stable`
- `proc memory_error` signature `proc memory_error() -> int {` example `memory.memory_error(...)` stability `stable`
- `proc is_null` signature `proc is_null() -> int {` example `memory.is_null(...)` stability `stable`
- `proc non_null` signature `proc non_null() -> int {` example `memory.non_null(...)` stability `stable`
- `proc align_up` signature `proc align_up() -> int {` example `memory.align_up(...)` stability `stable`
- `proc align_down` signature `proc align_down() -> int {` example `memory.align_down(...)` stability `stable`
- `proc is_aligned` signature `proc is_aligned() -> int {` example `memory.is_aligned(...)` stability `stable`
- `proc align_ptr` signature `proc align_ptr() -> int {` example `memory.align_ptr(...)` stability `stable`
- `proc page_align_up` signature `proc page_align_up() -> int {` example `memory.page_align_up(...)` stability `stable`
- `proc page_align_down` signature `proc page_align_down() -> int {` example `memory.page_align_down(...)` stability `stable`
- `proc page_index` signature `proc page_index() -> int {` example `memory.page_index(...)` stability `stable`
- `proc page_addr` signature `proc page_addr() -> int {` example `memory.page_addr(...)` stability `stable`
- `proc page_offset` signature `proc page_offset() -> int {` example `memory.page_offset(...)` stability `stable`
- `proc pages_for_size` signature `proc pages_for_size() -> int {` example `memory.pages_for_size(...)` stability `stable`
- `proc bytes_to_kb` signature `proc bytes_to_kb() -> int {` example `memory.bytes_to_kb(...)` stability `stable`
- `proc bytes_to_mb` signature `proc bytes_to_mb() -> int {` example `memory.bytes_to_mb(...)` stability `stable`
- `proc bytes_to_gb` signature `proc bytes_to_gb() -> int {` example `memory.bytes_to_gb(...)` stability `stable`
- `proc kb` signature `proc kb() -> int {` example `memory.kb(...)` stability `stable`
- `proc mb` signature `proc mb() -> int {` example `memory.mb(...)` stability `stable`
- `proc gb` signature `proc gb() -> int {` example `memory.gb(...)` stability `stable`
- `proc checked_add` signature `proc checked_add() -> int {` example `memory.checked_add(...)` stability `stable`
- `proc checked_mul` signature `proc checked_mul() -> int {` example `memory.checked_mul(...)` stability `stable`
- `proc ptr_add` signature `proc ptr_add() -> int {` example `memory.ptr_add(...)` stability `stable`
- `proc ptr_sub` signature `proc ptr_sub() -> int {` example `memory.ptr_sub(...)` stability `stable`
- `proc ptr_diff` signature `proc ptr_diff() -> int {` example `memory.ptr_diff(...)` stability `stable`
- `proc ptr_in_range` signature `proc ptr_in_range() -> int {` example `memory.ptr_in_range(...)` stability `stable`
- `proc ranges_overlap` signature `proc ranges_overlap() -> int {` example `memory.ranges_overlap(...)` stability `stable`
- `proc block_new` signature `proc block_new() -> int {` example `memory.block_new(...)` stability `stable`
- `proc block_empty` signature `proc block_empty() -> int {` example `memory.block_empty(...)` stability `stable`
- `proc block_end` signature `proc block_end() -> int {` example `memory.block_end(...)` stability `stable`
- `proc block_contains` signature `proc block_contains() -> int {` example `memory.block_contains(...)` stability `stable`
- `proc block_is_valid` signature `proc block_is_valid() -> int {` example `memory.block_is_valid(...)` stability `stable`
- `proc region_new` signature `proc region_new() -> int {` example `memory.region_new(...)` stability `stable`
- `proc region_end` signature `proc region_end() -> int {` example `memory.region_end(...)` stability `stable`
- `proc region_contains` signature `proc region_contains() -> int {` example `memory.region_contains(...)` stability `stable`
- `proc region_is_readable` signature `proc region_is_readable() -> int {` example `memory.region_is_readable(...)` stability `stable`
- `proc region_is_writable` signature `proc region_is_writable() -> int {` example `memory.region_is_writable(...)` stability `stable`
- `proc region_is_executable` signature `proc region_is_executable() -> int {` example `memory.region_is_executable(...)` stability `stable`
- `proc region_is_kernel` signature `proc region_is_kernel() -> int {` example `memory.region_is_kernel(...)` stability `stable`
- `proc region_is_user` signature `proc region_is_user() -> int {` example `memory.region_is_user(...)` stability `stable`
- `proc memory_map_empty` signature `proc memory_map_empty() -> int {` example `memory.memory_map_empty(...)` stability `stable`
- `proc memory_map_add` signature `proc memory_map_add() -> int {` example `memory.memory_map_add(...)` stability `stable`
- `proc memory_map_find` signature `proc memory_map_find() -> int {` example `memory.memory_map_find(...)` stability `stable`
- `proc memory_map_usable_regions` signature `proc memory_map_usable_regions() -> int {` example `memory.memory_map_usable_regions(...)` stability `stable`
- `proc heap_new` signature `proc heap_new() -> int {` example `memory.heap_new(...)` stability `stable`
- `proc heap_remaining` signature `proc heap_remaining() -> int {` example `memory.heap_remaining(...)` stability `stable`
- `proc heap_alloc` signature `proc heap_alloc() -> int {` example `memory.heap_alloc(...)` stability `stable`
- `proc heap_last_block` signature `proc heap_last_block() -> int {` example `memory.heap_last_block(...)` stability `stable`
- `proc heap_contains` signature `proc heap_contains() -> int {` example `memory.heap_contains(...)` stability `stable`
- `proc heap_reset` signature `proc heap_reset() -> int {` example `memory.heap_reset(...)` stability `stable`
- `proc heap_stats` signature `proc heap_stats() -> int {` example `memory.heap_stats(...)` stability `stable`
- `proc arena_new` signature `proc arena_new() -> int {` example `memory.arena_new(...)` stability `stable`
- `proc arena_remaining` signature `proc arena_remaining() -> int {` example `memory.arena_remaining(...)` stability `stable`
- `proc arena_current` signature `proc arena_current() -> int {` example `memory.arena_current(...)` stability `stable`
- `proc arena_alloc` signature `proc arena_alloc() -> int {` example `memory.arena_alloc(...)` stability `stable`
- `proc arena_alloc_aligned` signature `proc arena_alloc_aligned() -> int {` example `memory.arena_alloc_aligned(...)` stability `stable`
- `proc arena_last_addr` signature `proc arena_last_addr() -> int {` example `memory.arena_last_addr(...)` stability `stable`
- `proc arena_reset` signature `proc arena_reset() -> int {` example `memory.arena_reset(...)` stability `stable`
- `proc arena_mark` signature `proc arena_mark() -> int {` example `memory.arena_mark(...)` stability `stable`
- `proc arena_restore` signature `proc arena_restore() -> int {` example `memory.arena_restore(...)` stability `stable`
- `proc arena_used` signature `proc arena_used() -> int {` example `memory.arena_used(...)` stability `stable`
- `proc arena_full` signature `proc arena_full() -> int {` example `memory.arena_full(...)` stability `stable`
- `proc pool_new` signature `proc pool_new() -> int {` example `memory.pool_new(...)` stability `stable`
- `proc pool_block_addr` signature `proc pool_block_addr() -> int {` example `memory.pool_block_addr(...)` stability `stable`
- `proc pool_index_of` signature `proc pool_index_of() -> int {` example `memory.pool_index_of(...)` stability `stable`
- `proc pool_has_free` signature `proc pool_has_free() -> int {` example `memory.pool_has_free(...)` stability `stable`
- `proc pool_alloc` signature `proc pool_alloc() -> int {` example `memory.pool_alloc(...)` stability `stable`
- `proc pool_last_alloc_addr` signature `proc pool_last_alloc_addr() -> int {` example `memory.pool_last_alloc_addr(...)` stability `stable`
- `proc pool_free` signature `proc pool_free() -> int {` example `memory.pool_free(...)` stability `stable`
- `proc pool_used` signature `proc pool_used() -> int {` example `memory.pool_used(...)` stability `stable`
- `proc pool_free_count` signature `proc pool_free_count() -> int {` example `memory.pool_free_count(...)` stability `stable`
- `proc pool_capacity` signature `proc pool_capacity() -> int {` example `memory.pool_capacity(...)` stability `stable`
- `proc pool_contains` signature `proc pool_contains() -> int {` example `memory.pool_contains(...)` stability `stable`
- `proc slab_class` signature `proc slab_class() -> int {` example `memory.slab_class(...)` stability `stable`
- `proc slab_new` signature `proc slab_new() -> int {` example `memory.slab_new(...)` stability `stable`
- `proc slab_find_class` signature `proc slab_find_class() -> int {` example `memory.slab_find_class(...)` stability `stable`
- `proc slab_alloc` signature `proc slab_alloc() -> int {` example `memory.slab_alloc(...)` stability `stable`
- `proc slab_free` signature `proc slab_free() -> int {` example `memory.slab_free(...)` stability `stable`
- `proc slab_free_blocks` signature `proc slab_free_blocks() -> int {` example `memory.slab_free_blocks(...)` stability `stable`
- `proc page_new` signature `proc page_new() -> int {` example `memory.page_new(...)` stability `stable`
- `proc page_is_free` signature `proc page_is_free() -> int {` example `memory.page_is_free(...)` stability `stable`
- `proc page_is_used` signature `proc page_is_used() -> int {` example `memory.page_is_used(...)` stability `stable`
- `proc page_range` signature `proc page_range() -> int {` example `memory.page_range(...)` stability `stable`
- `proc page_range_end` signature `proc page_range_end() -> int {` example `memory.page_range_end(...)` stability `stable`
- `proc page_range_contains` signature `proc page_range_contains() -> int {` example `memory.page_range_contains(...)` stability `stable`
- `proc pages_make` signature `proc pages_make() -> int {` example `memory.pages_make(...)` stability `stable`
- `proc pages_mark_range` signature `proc pages_mark_range() -> int {` example `memory.pages_mark_range(...)` stability `stable`
- `proc pages_find_free_run` signature `proc pages_find_free_run() -> int {` example `memory.pages_find_free_run(...)` stability `stable`
- `proc pages_alloc` signature `proc pages_alloc() -> int {` example `memory.pages_alloc(...)` stability `stable`
- `proc pages_free` signature `proc pages_free() -> int {` example `memory.pages_free(...)` stability `stable`
- `proc bytes_empty` signature `proc bytes_empty() -> int {` example `memory.bytes_empty(...)` stability `stable`
- `proc bytes_len` signature `proc bytes_len() -> int {` example `memory.bytes_len(...)` stability `stable`
- `proc bytes_is_empty` signature `proc bytes_is_empty() -> int {` example `memory.bytes_is_empty(...)` stability `stable`
- `proc bytes_repeat` signature `proc bytes_repeat() -> int {` example `memory.bytes_repeat(...)` stability `stable`
- `proc bytes_zero` signature `proc bytes_zero() -> int {` example `memory.bytes_zero(...)` stability `stable`
- `proc bytes_fill` signature `proc bytes_fill() -> int {` example `memory.bytes_fill(...)` stability `stable`
- `proc bytes_slice` signature `proc bytes_slice() -> int {` example `memory.bytes_slice(...)` stability `stable`
- `proc bytes_concat` signature `proc bytes_concat() -> int {` example `memory.bytes_concat(...)` stability `stable`
- `proc bytes_get` signature `proc bytes_get() -> int {` example `memory.bytes_get(...)` stability `stable`
- `proc bytes_set` signature `proc bytes_set() -> int {` example `memory.bytes_set(...)` stability `stable`
- `proc bytes_equal` signature `proc bytes_equal() -> int {` example `memory.bytes_equal(...)` stability `stable`
- `proc memcmp` signature `proc memcmp() -> int {` example `memory.memcmp(...)` stability `stable`
- `proc memcpy` signature `proc memcpy() -> int {` example `memory.memcpy(...)` stability `stable`
- `proc memmove` signature `proc memmove() -> int {` example `memory.memmove(...)` stability `stable`
- `proc memset` signature `proc memset() -> int {` example `memory.memset(...)` stability `stable`
- `proc memzero` signature `proc memzero() -> int {` example `memory.memzero(...)` stability `stable`
- `proc memchr` signature `proc memchr() -> int {` example `memory.memchr(...)` stability `stable`
- `proc memrchr` signature `proc memrchr() -> int {` example `memory.memrchr(...)` stability `stable`
- `proc buffer_new` signature `proc buffer_new() -> int {` example `memory.buffer_new(...)` stability `stable`
- `proc buffer_from_bytes` signature `proc buffer_from_bytes() -> int {` example `memory.buffer_from_bytes(...)` stability `stable`
- `proc buffer_is_empty` signature `proc buffer_is_empty() -> int {` example `memory.buffer_is_empty(...)` stability `stable`
- `proc buffer_remaining` signature `proc buffer_remaining() -> int {` example `memory.buffer_remaining(...)` stability `stable`
- `proc buffer_clear` signature `proc buffer_clear() -> int {` example `memory.buffer_clear(...)` stability `stable`
- `proc buffer_reserve` signature `proc buffer_reserve() -> int {` example `memory.buffer_reserve(...)` stability `stable`
- `proc buffer_push_byte` signature `proc buffer_push_byte() -> int {` example `memory.buffer_push_byte(...)` stability `stable`
- `proc buffer_append` signature `proc buffer_append() -> int {` example `memory.buffer_append(...)` stability `stable`
- `proc buffer_slice` signature `proc buffer_slice() -> int {` example `memory.buffer_slice(...)` stability `stable`
- `proc buffer_to_bytes` signature `proc buffer_to_bytes() -> int {` example `memory.buffer_to_bytes(...)` stability `stable`
- `proc array_fill_int` signature `proc array_fill_int() -> int {` example `memory.array_fill_int(...)` stability `stable`
- `proc memory_stats_empty` signature `proc memory_stats_empty() -> int {` example `memory.memory_stats_empty(...)` stability `stable`
- `proc memory_stats_from_heap` signature `proc memory_stats_from_heap() -> int {` example `memory.memory_stats_from_heap(...)` stability `stable`
- `proc memory_stats_percent_used` signature `proc memory_stats_percent_used() -> int {` example `memory.memory_stats_percent_used(...)` stability `stable`
- `proc memory_format_size` signature `proc memory_format_size() -> int {` example `memory.memory_format_size(...)` stability `stable`
- `proc memory_check_block` signature `proc memory_check_block() -> int {` example `memory.memory_check_block(...)` stability `stable`
- `proc memory_check_region` signature `proc memory_check_region() -> int {` example `memory.memory_check_region(...)` stability `stable`
- `proc memory_zero_block` signature `proc memory_zero_block() -> int {` example `memory.memory_zero_block(...)` stability `stable`
- `proc memory_copy_block` signature `proc memory_copy_block() -> int {` example `memory.memory_copy_block(...)` stability `stable`
- `proc memory_domains` signature `proc memory_domains() -> int {` example `memory.memory_domains(...)` stability `stable`
- `proc memory_ready` signature `proc memory_ready() -> int {` example `memory.memory_ready(...)` stability `stable`
- `proc library_meta` signature `proc library_meta() -> int {` example `memory.library_meta(...)` stability `stable`
- `proc memory_selftest_alignment` signature `proc memory_selftest_alignment() -> int {` example `memory.memory_selftest_alignment(...)` stability `stable`
- `proc memory_selftest_bytes` signature `proc memory_selftest_bytes() -> int {` example `memory.memory_selftest_bytes(...)` stability `stable`
- `proc memory_selftest_heap` signature `proc memory_selftest_heap() -> int {` example `memory.memory_selftest_heap(...)` stability `stable`
- `proc memory_selftest_arena` signature `proc memory_selftest_arena() -> int {` example `memory.memory_selftest_arena(...)` stability `stable`
- `proc memory_selftest_pool` signature `proc memory_selftest_pool() -> int {` example `memory.memory_selftest_pool(...)` stability `stable`
- `proc memory_selftest_pages` signature `proc memory_selftest_pages() -> int {` example `memory.memory_selftest_pages(...)` stability `stable`
- `proc memory_selftest` signature `proc memory_selftest() -> int {` example `memory.memory_selftest(...)` stability `stable`

## `src/vitte/stdlib/collections.vitl`

Stability: `stable`

- `const COLLECTION_OK` signature `const COLLECTION_OK: i32 = 0` example `collections.COLLECTION_OK` stability `stable`
- `const COLLECTION_ERR` signature `const COLLECTION_ERR: i32 = 0` example `collections.COLLECTION_ERR` stability `stable`
- `const COLLECTION_ERR_EMPTY` signature `const COLLECTION_ERR_EMPTY: i32 = 0` example `collections.COLLECTION_ERR_EMPTY` stability `stable`
- `const COLLECTION_ERR_BOUNDS` signature `const COLLECTION_ERR_BOUNDS: i32 = 0` example `collections.COLLECTION_ERR_BOUNDS` stability `stable`
- `const COLLECTION_ERR_NOT_FOUND` signature `const COLLECTION_ERR_NOT_FOUND: i32 = 0` example `collections.COLLECTION_ERR_NOT_FOUND` stability `stable`
- `const COLLECTION_ERR_FULL` signature `const COLLECTION_ERR_FULL: i32 = 0` example `collections.COLLECTION_ERR_FULL` stability `stable`
- `const COLLECTION_ERR_DUPLICATE` signature `const COLLECTION_ERR_DUPLICATE: i32 = 0` example `collections.COLLECTION_ERR_DUPLICATE` stability `stable`
- `const DEFAULT_CAPACITY` signature `const DEFAULT_CAPACITY: i32 = 0` example `collections.DEFAULT_CAPACITY` stability `stable`
- `const GROWTH_FACTOR` signature `const GROWTH_FACTOR: i32 = 0` example `collections.GROWTH_FACTOR` stability `stable`
- `const HASHMAP_MAX_LOAD_NUM` signature `const HASHMAP_MAX_LOAD_NUM: i32 = 0` example `collections.HASHMAP_MAX_LOAD_NUM` stability `stable`
- `const HASHMAP_MAX_LOAD_DEN` signature `const HASHMAP_MAX_LOAD_DEN: i32 = 0` example `collections.HASHMAP_MAX_LOAD_DEN` stability `stable`
- `form OptionI64` signature `form OptionI64 {` example `collections.OptionI64` stability `stable`
- `form OptionString` signature `form OptionString {` example `collections.OptionString` stability `stable`
- `form CollectionResult` signature `form CollectionResult {` example `collections.CollectionResult` stability `stable`
- `form Vector` signature `form Vector {` example `collections.Vector` stability `stable`
- `form Pair` signature `form Pair {` example `collections.Pair` stability `stable`
- `form Indexed` signature `form Indexed {` example `collections.Indexed` stability `stable`
- `form Range` signature `form Range {` example `collections.Range` stability `stable`
- `form ListNode` signature `form ListNode {` example `collections.ListNode` stability `stable`
- `form LinkedList` signature `form LinkedList {` example `collections.LinkedList` stability `stable`
- `form Queue` signature `form Queue {` example `collections.Queue` stability `stable`
- `form Deque` signature `form Deque {` example `collections.Deque` stability `stable`
- `form Stack` signature `form Stack {` example `collections.Stack` stability `stable`
- `form HashEntry` signature `form HashEntry {` example `collections.HashEntry` stability `stable`
- `form HashMap` signature `form HashMap {` example `collections.HashMap` stability `stable`
- `form HashSet` signature `form HashSet {` example `collections.HashSet` stability `stable`
- `form BinaryHeap` signature `form BinaryHeap {` example `collections.BinaryHeap` stability `stable`
- `pick Ordering` signature `pick Ordering {` example `collections.Ordering` stability `stable`
- `proc collection_error_name` signature `proc collection_error_name() -> int {` example `collections.collection_error_name(...)` stability `stable`
- `proc option_i64_none` signature `proc option_i64_none() -> int {` example `collections.option_i64_none(...)` stability `stable`
- `proc option_i64_some` signature `proc option_i64_some() -> int {` example `collections.option_i64_some(...)` stability `stable`
- `proc option_string_none` signature `proc option_string_none() -> int {` example `collections.option_string_none(...)` stability `stable`
- `proc option_string_some` signature `proc option_string_some() -> int {` example `collections.option_string_some(...)` stability `stable`
- `proc vector_new` signature `proc vector_new() -> int {` example `collections.vector_new(...)` stability `stable`
- `proc vector_with_capacity` signature `proc vector_with_capacity() -> int {` example `collections.vector_with_capacity(...)` stability `stable`
- `proc vector_from_array` signature `proc vector_from_array() -> int {` example `collections.vector_from_array(...)` stability `stable`
- `proc vector_len` signature `proc vector_len() -> int {` example `collections.vector_len(...)` stability `stable`
- `proc vector_size` signature `proc vector_size() -> int {` example `collections.vector_size(...)` stability `stable`
- `proc vector_capacity` signature `proc vector_capacity() -> int {` example `collections.vector_capacity(...)` stability `stable`
- `proc vector_is_empty` signature `proc vector_is_empty() -> int {` example `collections.vector_is_empty(...)` stability `stable`
- `proc vector_is_valid_index` signature `proc vector_is_valid_index() -> int {` example `collections.vector_is_valid_index(...)` stability `stable`
- `proc vector_reserve` signature `proc vector_reserve() -> int {` example `collections.vector_reserve(...)` stability `stable`
- `proc vector_push` signature `proc vector_push() -> int {` example `collections.vector_push(...)` stability `stable`
- `proc vector_pop_i64` signature `proc vector_pop_i64() -> int {` example `collections.vector_pop_i64(...)` stability `stable`
- `proc vector_pop` signature `proc vector_pop() -> int {` example `collections.vector_pop(...)` stability `stable`
- `proc vector_get_i64` signature `proc vector_get_i64() -> int {` example `collections.vector_get_i64(...)` stability `stable`
- `proc vector_at_i64` signature `proc vector_at_i64() -> int {` example `collections.vector_at_i64(...)` stability `stable`
- `proc vector_set_i64` signature `proc vector_set_i64() -> int {` example `collections.vector_set_i64(...)` stability `stable`
- `proc vector_insert_i64` signature `proc vector_insert_i64() -> int {` example `collections.vector_insert_i64(...)` stability `stable`
- `proc vector_remove_i64` signature `proc vector_remove_i64() -> int {` example `collections.vector_remove_i64(...)` stability `stable`
- `proc vector_clear` signature `proc vector_clear() -> int {` example `collections.vector_clear(...)` stability `stable`
- `proc vector_to_array` signature `proc vector_to_array() -> int {` example `collections.vector_to_array(...)` stability `stable`
- `proc vector_clone` signature `proc vector_clone() -> int {` example `collections.vector_clone(...)` stability `stable`
- `proc vector_extend` signature `proc vector_extend() -> int {` example `collections.vector_extend(...)` stability `stable`
- `proc vector_contains_i64` signature `proc vector_contains_i64() -> int {` example `collections.vector_contains_i64(...)` stability `stable`
- `proc vector_index_of_i64` signature `proc vector_index_of_i64() -> int {` example `collections.vector_index_of_i64(...)` stability `stable`
- `proc vector_reverse_i64` signature `proc vector_reverse_i64() -> int {` example `collections.vector_reverse_i64(...)` stability `stable`
- `proc vector_sum_i64` signature `proc vector_sum_i64() -> int {` example `collections.vector_sum_i64(...)` stability `stable`
- `proc vector_min_i64` signature `proc vector_min_i64() -> int {` example `collections.vector_min_i64(...)` stability `stable`
- `proc vector_max_i64` signature `proc vector_max_i64() -> int {` example `collections.vector_max_i64(...)` stability `stable`
- `proc vector_sort_i64` signature `proc vector_sort_i64() -> int {` example `collections.vector_sort_i64(...)` stability `stable`
- `proc vector_binary_search_i64` signature `proc vector_binary_search_i64() -> int {` example `collections.vector_binary_search_i64(...)` stability `stable`
- `proc copy` signature `proc copy() -> int {` example `collections.copy(...)` stability `stable`
- `proc list_new` signature `proc list_new() -> int {` example `collections.list_new(...)` stability `stable`
- `proc list_len` signature `proc list_len() -> int {` example `collections.list_len(...)` stability `stable`
- `proc list_size` signature `proc list_size() -> int {` example `collections.list_size(...)` stability `stable`
- `proc list_is_empty` signature `proc list_is_empty() -> int {` example `collections.list_is_empty(...)` stability `stable`
- `proc list_push_back` signature `proc list_push_back() -> int {` example `collections.list_push_back(...)` stability `stable`
- `proc list_push_front` signature `proc list_push_front() -> int {` example `collections.list_push_front(...)` stability `stable`
- `proc list_clear` signature `proc list_clear() -> int {` example `collections.list_clear(...)` stability `stable`
- `proc list_to_array` signature `proc list_to_array() -> int {` example `collections.list_to_array(...)` stability `stable`
- `proc queue_new` signature `proc queue_new() -> int {` example `collections.queue_new(...)` stability `stable`
- `proc queue_len` signature `proc queue_len() -> int {` example `collections.queue_len(...)` stability `stable`
- `proc queue_size` signature `proc queue_size() -> int {` example `collections.queue_size(...)` stability `stable`
- `proc queue_is_empty` signature `proc queue_is_empty() -> int {` example `collections.queue_is_empty(...)` stability `stable`
- `proc queue_is_full` signature `proc queue_is_full() -> int {` example `collections.queue_is_full(...)` stability `stable`
- `proc queue_enqueue` signature `proc queue_enqueue() -> int {` example `collections.queue_enqueue(...)` stability `stable`
- `proc queue_dequeue_i64` signature `proc queue_dequeue_i64() -> int {` example `collections.queue_dequeue_i64(...)` stability `stable`
- `proc queue_front_i64` signature `proc queue_front_i64() -> int {` example `collections.queue_front_i64(...)` stability `stable`
- `proc queue_clear` signature `proc queue_clear() -> int {` example `collections.queue_clear(...)` stability `stable`
- `proc deque_new` signature `proc deque_new() -> int {` example `collections.deque_new(...)` stability `stable`
- `proc deque_is_empty` signature `proc deque_is_empty() -> int {` example `collections.deque_is_empty(...)` stability `stable`
- `proc deque_is_full` signature `proc deque_is_full() -> int {` example `collections.deque_is_full(...)` stability `stable`
- `proc deque_push_back` signature `proc deque_push_back() -> int {` example `collections.deque_push_back(...)` stability `stable`
- `proc deque_push_front_i64` signature `proc deque_push_front_i64() -> int {` example `collections.deque_push_front_i64(...)` stability `stable`
- `proc deque_pop_back_i64` signature `proc deque_pop_back_i64() -> int {` example `collections.deque_pop_back_i64(...)` stability `stable`
- `proc deque_pop_front_i64` signature `proc deque_pop_front_i64() -> int {` example `collections.deque_pop_front_i64(...)` stability `stable`
- `proc stack_new` signature `proc stack_new() -> int {` example `collections.stack_new(...)` stability `stable`
- `proc stack_len` signature `proc stack_len() -> int {` example `collections.stack_len(...)` stability `stable`
- `proc stack_size` signature `proc stack_size() -> int {` example `collections.stack_size(...)` stability `stable`
- `proc stack_is_empty` signature `proc stack_is_empty() -> int {` example `collections.stack_is_empty(...)` stability `stable`
- `proc stack_is_full` signature `proc stack_is_full() -> int {` example `collections.stack_is_full(...)` stability `stable`
- `proc stack_push` signature `proc stack_push() -> int {` example `collections.stack_push(...)` stability `stable`
- `proc stack_pop_i64` signature `proc stack_pop_i64() -> int {` example `collections.stack_pop_i64(...)` stability `stable`
- `proc stack_peek_i64` signature `proc stack_peek_i64() -> int {` example `collections.stack_peek_i64(...)` stability `stable`
- `proc stack_clear` signature `proc stack_clear() -> int {` example `collections.stack_clear(...)` stability `stable`
- `proc hashmap_new` signature `proc hashmap_new() -> int {` example `collections.hashmap_new(...)` stability `stable`
- `proc hashmap_empty_entries` signature `proc hashmap_empty_entries() -> int {` example `collections.hashmap_empty_entries(...)` stability `stable`
- `proc hashmap_len` signature `proc hashmap_len() -> int {` example `collections.hashmap_len(...)` stability `stable`
- `proc hashmap_size` signature `proc hashmap_size() -> int {` example `collections.hashmap_size(...)` stability `stable`
- `proc hashmap_is_empty` signature `proc hashmap_is_empty() -> int {` example `collections.hashmap_is_empty(...)` stability `stable`
- `proc hashmap_load_percent` signature `proc hashmap_load_percent() -> int {` example `collections.hashmap_load_percent(...)` stability `stable`
- `proc hashmap_should_grow` signature `proc hashmap_should_grow() -> int {` example `collections.hashmap_should_grow(...)` stability `stable`
- `proc hash_string` signature `proc hash_string() -> int {` example `collections.hash_string(...)` stability `stable`
- `proc hashmap_find_slot` signature `proc hashmap_find_slot() -> int {` example `collections.hashmap_find_slot(...)` stability `stable`
- `proc hashmap_contains` signature `proc hashmap_contains() -> int {` example `collections.hashmap_contains(...)` stability `stable`
- `proc hashmap_insert` signature `proc hashmap_insert() -> int {` example `collections.hashmap_insert(...)` stability `stable`
- `proc hashmap_remove` signature `proc hashmap_remove() -> int {` example `collections.hashmap_remove(...)` stability `stable`
- `proc hashmap_clear` signature `proc hashmap_clear() -> int {` example `collections.hashmap_clear(...)` stability `stable`
- `proc hashmap_rehash` signature `proc hashmap_rehash() -> int {` example `collections.hashmap_rehash(...)` stability `stable`
- `proc hashmap_keys` signature `proc hashmap_keys() -> int {` example `collections.hashmap_keys(...)` stability `stable`
- `proc hashmap_get_i64` signature `proc hashmap_get_i64() -> int {` example `collections.hashmap_get_i64(...)` stability `stable`
- `proc hashmap_values_i64` signature `proc hashmap_values_i64() -> int {` example `collections.hashmap_values_i64(...)` stability `stable`
- `proc hashmap_items_i64` signature `proc hashmap_items_i64() -> int {` example `collections.hashmap_items_i64(...)` stability `stable`
- `proc hashset_new` signature `proc hashset_new() -> int {` example `collections.hashset_new(...)` stability `stable`
- `proc hashset_with_capacity` signature `proc hashset_with_capacity() -> int {` example `collections.hashset_with_capacity(...)` stability `stable`
- `proc hashset_insert` signature `proc hashset_insert() -> int {` example `collections.hashset_insert(...)` stability `stable`
- `proc hashset_remove` signature `proc hashset_remove() -> int {` example `collections.hashset_remove(...)` stability `stable`
- `proc hashset_contains` signature `proc hashset_contains() -> int {` example `collections.hashset_contains(...)` stability `stable`
- `proc hashset_size` signature `proc hashset_size() -> int {` example `collections.hashset_size(...)` stability `stable`
- `proc hashset_len` signature `proc hashset_len() -> int {` example `collections.hashset_len(...)` stability `stable`
- `proc hashset_is_empty` signature `proc hashset_is_empty() -> int {` example `collections.hashset_is_empty(...)` stability `stable`
- `proc hashset_clear` signature `proc hashset_clear() -> int {` example `collections.hashset_clear(...)` stability `stable`
- `proc hashset_values` signature `proc hashset_values() -> int {` example `collections.hashset_values(...)` stability `stable`
- `proc hashset_union` signature `proc hashset_union() -> int {` example `collections.hashset_union(...)` stability `stable`
- `proc hashset_intersection` signature `proc hashset_intersection() -> int {` example `collections.hashset_intersection(...)` stability `stable`
- `proc hashset_difference` signature `proc hashset_difference() -> int {` example `collections.hashset_difference(...)` stability `stable`
- `proc heap_new` signature `proc heap_new() -> int {` example `collections.heap_new(...)` stability `stable`
- `proc heap_len` signature `proc heap_len() -> int {` example `collections.heap_len(...)` stability `stable`
- `proc heap_is_empty` signature `proc heap_is_empty() -> int {` example `collections.heap_is_empty(...)` stability `stable`
- `proc min_heap_i64` signature `proc min_heap_i64() -> int {` example `collections.min_heap_i64(...)` stability `stable`
- `proc max_heap_i64` signature `proc max_heap_i64() -> int {` example `collections.max_heap_i64(...)` stability `stable`
- `proc heap_should_swap_i64` signature `proc heap_should_swap_i64() -> int {` example `collections.heap_should_swap_i64(...)` stability `stable`
- `proc heap_push_i64` signature `proc heap_push_i64() -> int {` example `collections.heap_push_i64(...)` stability `stable`
- `proc heap_peek_i64` signature `proc heap_peek_i64() -> int {` example `collections.heap_peek_i64(...)` stability `stable`
- `proc range_new` signature `proc range_new() -> int {` example `collections.range_new(...)` stability `stable`
- `proc range_len` signature `proc range_len() -> int {` example `collections.range_len(...)` stability `stable`
- `proc range_to_array` signature `proc range_to_array() -> int {` example `collections.range_to_array(...)` stability `stable`
- `proc range` signature `proc range() -> int {` example `collections.range(...)` stability `stable`
- `proc range2` signature `proc range2() -> int {` example `collections.range2(...)` stability `stable`
- `proc range3` signature `proc range3() -> int {` example `collections.range3(...)` stability `stable`
- `proc zip` signature `proc zip() -> int {` example `collections.zip(...)` stability `stable`
- `proc enumerate` signature `proc enumerate() -> int {` example `collections.enumerate(...)` stability `stable`
- `proc reversed` signature `proc reversed() -> int {` example `collections.reversed(...)` stability `stable`
- `proc array_is_empty` signature `proc array_is_empty() -> int {` example `collections.array_is_empty(...)` stability `stable`
- `proc array_first_i64` signature `proc array_first_i64() -> int {` example `collections.array_first_i64(...)` stability `stable`
- `proc array_last_i64` signature `proc array_last_i64() -> int {` example `collections.array_last_i64(...)` stability `stable`
- `proc array_sum_i64` signature `proc array_sum_i64() -> int {` example `collections.array_sum_i64(...)` stability `stable`
- `proc array_product_i64` signature `proc array_product_i64() -> int {` example `collections.array_product_i64(...)` stability `stable`
- `proc array_contains_i64` signature `proc array_contains_i64() -> int {` example `collections.array_contains_i64(...)` stability `stable`
- `proc array_index_of_i64` signature `proc array_index_of_i64() -> int {` example `collections.array_index_of_i64(...)` stability `stable`
- `proc array_count_i64` signature `proc array_count_i64() -> int {` example `collections.array_count_i64(...)` stability `stable`
- `proc array_unique_i64` signature `proc array_unique_i64() -> int {` example `collections.array_unique_i64(...)` stability `stable`
- `proc array_filter_nonzero_i64` signature `proc array_filter_nonzero_i64() -> int {` example `collections.array_filter_nonzero_i64(...)` stability `stable`
- `proc array_map_double_i64` signature `proc array_map_double_i64() -> int {` example `collections.array_map_double_i64(...)` stability `stable`
- `proc array_sort_i64` signature `proc array_sort_i64() -> int {` example `collections.array_sort_i64(...)` stability `stable`
- `proc normalize_capacity` signature `proc normalize_capacity() -> int {` example `collections.normalize_capacity(...)` stability `stable`
- `proc min_i32` signature `proc min_i32() -> int {` example `collections.min_i32(...)` stability `stable`
- `proc max_i32` signature `proc max_i32() -> int {` example `collections.max_i32(...)` stability `stable`
- `proc clamp_i32` signature `proc clamp_i32() -> int {` example `collections.clamp_i32(...)` stability `stable`
- `proc compare_i64` signature `proc compare_i64() -> int {` example `collections.compare_i64(...)` stability `stable`
- `proc zero_value` signature `proc zero_value() -> int {` example `collections.zero_value(...)` stability `stable`
- `proc array_len` signature `proc array_len() -> int {` example `collections.array_len(...)` stability `stable`
- `proc array_copy` signature `proc array_copy() -> int {` example `collections.array_copy(...)` stability `stable`
- `proc array_get` signature `proc array_get() -> int {` example `collections.array_get(...)` stability `stable`
- `proc array_set` signature `proc array_set() -> int {` example `collections.array_set(...)` stability `stable`
- `proc array_push` signature `proc array_push() -> int {` example `collections.array_push(...)` stability `stable`
- `proc array_take` signature `proc array_take() -> int {` example `collections.array_take(...)` stability `stable`
- `proc array_drop` signature `proc array_drop() -> int {` example `collections.array_drop(...)` stability `stable`
- `proc array_get_i64` signature `proc array_get_i64() -> int {` example `collections.array_get_i64(...)` stability `stable`
- `proc array_set_i64` signature `proc array_set_i64() -> int {` example `collections.array_set_i64(...)` stability `stable`
- `proc array_get_string` signature `proc array_get_string() -> int {` example `collections.array_get_string(...)` stability `stable`
- `proc string_len` signature `proc string_len() -> int {` example `collections.string_len(...)` stability `stable`
- `proc string_char_code` signature `proc string_char_code() -> int {` example `collections.string_char_code(...)` stability `stable`
- `proc stdlib_smoke_6` signature `proc stdlib_smoke_6() -> int {` example `collections.stdlib_smoke_6(...)` stability `stable`
- `proc stdlib_smoke_7` signature `proc stdlib_smoke_7() -> int {` example `collections.stdlib_smoke_7(...)` stability `stable`
- `proc stdlib_smoke_8` signature `proc stdlib_smoke_8() -> int {` example `collections.stdlib_smoke_8(...)` stability `stable`
- `proc stdlib_smoke_9` signature `proc stdlib_smoke_9() -> int {` example `collections.stdlib_smoke_9(...)` stability `stable`
- `proc stdlib_smoke_10` signature `proc stdlib_smoke_10() -> int {` example `collections.stdlib_smoke_10(...)` stability `stable`
- `proc stdlib_smoke_11` signature `proc stdlib_smoke_11() -> int {` example `collections.stdlib_smoke_11(...)` stability `stable`

## `src/vitte/stdlib/math.vitl`

Stability: `stable`

- `const PI` signature `const PI: f64 = 0.0` example `math.PI` stability `stable`
- `const E` signature `const E: f64 = 0.0` example `math.E` stability `stable`
- `const TAU` signature `const TAU: f64 = 0.0` example `math.TAU` stability `stable`
- `const HALF_PI` signature `const HALF_PI: f64 = 0.0` example `math.HALF_PI` stability `stable`
- `const QUARTER_PI` signature `const QUARTER_PI: f64 = 0.0` example `math.QUARTER_PI` stability `stable`
- `const DEG_TO_RAD` signature `const DEG_TO_RAD: f64 = 0.0` example `math.DEG_TO_RAD` stability `stable`
- `const RAD_TO_DEG` signature `const RAD_TO_DEG: f64 = 0.0` example `math.RAD_TO_DEG` stability `stable`
- `const I32_MIN` signature `const I32_MIN: int = 0` example `math.I32_MIN` stability `stable`
- `const I32_MAX` signature `const I32_MAX: int = 0` example `math.I32_MAX` stability `stable`
- `const BYTE_BITS` signature `const BYTE_BITS: int = 0` example `math.BYTE_BITS` stability `stable`
- `const WORD_BITS` signature `const WORD_BITS: int = 0` example `math.WORD_BITS` stability `stable`
- `const DEFAULT_SCALE` signature `const DEFAULT_SCALE: int = 0` example `math.DEFAULT_SCALE` stability `stable`
- `const PERCENT_SCALE` signature `const PERCENT_SCALE: int = 0` example `math.PERCENT_SCALE` stability `stable`
- `form Ratio` signature `form Ratio {` example `math.Ratio` stability `stable`
- `form IntRange` signature `form IntRange {` example `math.IntRange` stability `stable`
- `form IntStats` signature `form IntStats {` example `math.IntStats` stability `stable`
- `form Point2i` signature `form Point2i {` example `math.Point2i` stability `stable`
- `form Size2i` signature `form Size2i {` example `math.Size2i` stability `stable`
- `form Rect2i` signature `form Rect2i {` example `math.Rect2i` stability `stable`
- `form Vec2i` signature `form Vec2i {` example `math.Vec2i` stability `stable`
- `form Vec3i` signature `form Vec3i {` example `math.Vec3i` stability `stable`
- `form MathManifest` signature `form MathManifest {` example `math.MathManifest` stability `stable`
- `form MathHealth` signature `form MathHealth {` example `math.MathHealth` stability `stable`
- `form MathSummary` signature `form MathSummary {` example `math.MathSummary` stability `stable`
- `pick MathError` signature `pick MathError {` example `math.MathError` stability `stable`
- `proc math_error_code` signature `proc math_error_code() -> int {` example `math.math_error_code(...)` stability `stable`
- `proc math_version` signature `proc math_version() -> int {` example `math.math_version(...)` stability `stable`
- `proc math_name` signature `proc math_name() -> int {` example `math.math_name(...)` stability `stable`
- `proc math_module_count` signature `proc math_module_count() -> int {` example `math.math_module_count(...)` stability `stable`
- `proc math_modules` signature `proc math_modules() -> int {` example `math.math_modules(...)` stability `stable`
- `proc math_manifest` signature `proc math_manifest() -> int {` example `math.math_manifest(...)` stability `stable`
- `proc add` signature `proc add() -> int {` example `math.add(...)` stability `stable`
- `proc sub` signature `proc sub() -> int {` example `math.sub(...)` stability `stable`
- `proc mul` signature `proc mul() -> int {` example `math.mul(...)` stability `stable`
- `proc div_trunc` signature `proc div_trunc() -> int {` example `math.div_trunc(...)` stability `stable`
- `proc rem` signature `proc rem() -> int {` example `math.rem(...)` stability `stable`
- `proc abs` signature `proc abs() -> int {` example `math.abs(...)` stability `stable`
- `proc min` signature `proc min() -> int {` example `math.min(...)` stability `stable`
- `proc max` signature `proc max() -> int {` example `math.max(...)` stability `stable`
- `proc clamp` signature `proc clamp() -> int {` example `math.clamp(...)` stability `stable`
- `proc sign` signature `proc sign() -> int {` example `math.sign(...)` stability `stable`
- `proc between` signature `proc between() -> int {` example `math.between(...)` stability `stable`
- `proc compare` signature `proc compare() -> int {` example `math.compare(...)` stability `stable`
- `proc neg` signature `proc neg() -> int {` example `math.neg(...)` stability `stable`
- `proc inc` signature `proc inc() -> int {` example `math.inc(...)` stability `stable`
- `proc dec` signature `proc dec() -> int {` example `math.dec(...)` stability `stable`
- `proc square` signature `proc square() -> int {` example `math.square(...)` stability `stable`
- `proc cube` signature `proc cube() -> int {` example `math.cube(...)` stability `stable`
- `proc avg2` signature `proc avg2() -> int {` example `math.avg2(...)` stability `stable`
- `proc midpoint` signature `proc midpoint() -> int {` example `math.midpoint(...)` stability `stable`
- `proc safe_div` signature `proc safe_div() -> int {` example `math.safe_div(...)` stability `stable`
- `proc safe_rem` signature `proc safe_rem() -> int {` example `math.safe_rem(...)` stability `stable`
- `proc saturating_add` signature `proc saturating_add() -> int {` example `math.saturating_add(...)` stability `stable`
- `proc saturating_sub` signature `proc saturating_sub() -> int {` example `math.saturating_sub(...)` stability `stable`
- `proc saturating_mul` signature `proc saturating_mul() -> int {` example `math.saturating_mul(...)` stability `stable`
- `proc checked_add_valid` signature `proc checked_add_valid() -> int {` example `math.checked_add_valid(...)` stability `stable`
- `proc checked_mul_valid` signature `proc checked_mul_valid() -> int {` example `math.checked_mul_valid(...)` stability `stable`
- `proc gcd` signature `proc gcd() -> int {` example `math.gcd(...)` stability `stable`
- `proc lcm` signature `proc lcm() -> int {` example `math.lcm(...)` stability `stable`
- `proc gcd_many` signature `proc gcd_many() -> int {` example `math.gcd_many(...)` stability `stable`
- `proc lcm_many` signature `proc lcm_many() -> int {` example `math.lcm_many(...)` stability `stable`
- `proc is_even` signature `proc is_even() -> int {` example `math.is_even(...)` stability `stable`
- `proc is_odd` signature `proc is_odd() -> int {` example `math.is_odd(...)` stability `stable`
- `proc is_prime` signature `proc is_prime() -> int {` example `math.is_prime(...)` stability `stable`
- `proc is_composite` signature `proc is_composite() -> int {` example `math.is_composite(...)` stability `stable`
- `proc next_prime` signature `proc next_prime() -> int {` example `math.next_prime(...)` stability `stable`
- `proc prev_prime` signature `proc prev_prime() -> int {` example `math.prev_prime(...)` stability `stable`
- `proc prime_factors` signature `proc prime_factors() -> int {` example `math.prime_factors(...)` stability `stable`
- `proc divisors` signature `proc divisors() -> int {` example `math.divisors(...)` stability `stable`
- `proc totient` signature `proc totient() -> int {` example `math.totient(...)` stability `stable`
- `proc coprime` signature `proc coprime() -> int {` example `math.coprime(...)` stability `stable`
- `proc pow_int` signature `proc pow_int() -> int {` example `math.pow_int(...)` stability `stable`
- `proc pow2` signature `proc pow2() -> int {` example `math.pow2(...)` stability `stable`
- `proc pow10` signature `proc pow10() -> int {` example `math.pow10(...)` stability `stable`
- `proc factorial` signature `proc factorial() -> int {` example `math.factorial(...)` stability `stable`
- `proc triangular` signature `proc triangular() -> int {` example `math.triangular(...)` stability `stable`
- `proc fibonacci` signature `proc fibonacci() -> int {` example `math.fibonacci(...)` stability `stable`
- `proc binomial` signature `proc binomial() -> int {` example `math.binomial(...)` stability `stable`
- `proc falling_factorial` signature `proc falling_factorial() -> int {` example `math.falling_factorial(...)` stability `stable`
- `proc rising_factorial` signature `proc rising_factorial() -> int {` example `math.rising_factorial(...)` stability `stable`
- `proc double_factorial` signature `proc double_factorial() -> int {` example `math.double_factorial(...)` stability `stable`
- `proc sqrt_floor` signature `proc sqrt_floor() -> int {` example `math.sqrt_floor(...)` stability `stable`
- `proc sqrt_ceil` signature `proc sqrt_ceil() -> int {` example `math.sqrt_ceil(...)` stability `stable`
- `proc cube_root_floor` signature `proc cube_root_floor() -> int {` example `math.cube_root_floor(...)` stability `stable`
- `proc cube_root_ceil` signature `proc cube_root_ceil() -> int {` example `math.cube_root_ceil(...)` stability `stable`
- `proc is_square` signature `proc is_square() -> int {` example `math.is_square(...)` stability `stable`
- `proc is_cube` signature `proc is_cube() -> int {` example `math.is_cube(...)` stability `stable`
- `proc ilog2_floor` signature `proc ilog2_floor() -> int {` example `math.ilog2_floor(...)` stability `stable`
- `proc ilog10_floor` signature `proc ilog10_floor() -> int {` example `math.ilog10_floor(...)` stability `stable`
- `proc ceil_div` signature `proc ceil_div() -> int {` example `math.ceil_div(...)` stability `stable`
- `proc floor_div` signature `proc floor_div() -> int {` example `math.floor_div(...)` stability `stable`
- `proc floor_mod` signature `proc floor_mod() -> int {` example `math.floor_mod(...)` stability `stable`
- `proc div_round_nearest` signature `proc div_round_nearest() -> int {` example `math.div_round_nearest(...)` stability `stable`
- `proc mod_normalize` signature `proc mod_normalize() -> int {` example `math.mod_normalize(...)` stability `stable`
- `proc mod_add` signature `proc mod_add() -> int {` example `math.mod_add(...)` stability `stable`
- `proc mod_sub` signature `proc mod_sub() -> int {` example `math.mod_sub(...)` stability `stable`
- `proc mod_mul` signature `proc mod_mul() -> int {` example `math.mod_mul(...)` stability `stable`
- `proc mod_pow` signature `proc mod_pow() -> int {` example `math.mod_pow(...)` stability `stable`
- `proc mod_neg` signature `proc mod_neg() -> int {` example `math.mod_neg(...)` stability `stable`
- `proc mod_inverse` signature `proc mod_inverse() -> int {` example `math.mod_inverse(...)` stability `stable`
- `proc mod_div` signature `proc mod_div() -> int {` example `math.mod_div(...)` stability `stable`
- `proc is_pow2` signature `proc is_pow2() -> int {` example `math.is_pow2(...)` stability `stable`
- `proc prev_pow2` signature `proc prev_pow2() -> int {` example `math.prev_pow2(...)` stability `stable`
- `proc next_pow2` signature `proc next_pow2() -> int {` example `math.next_pow2(...)` stability `stable`
- `proc has_single_bit` signature `proc has_single_bit() -> int {` example `math.has_single_bit(...)` stability `stable`
- `proc bit_floor` signature `proc bit_floor() -> int {` example `math.bit_floor(...)` stability `stable`
- `proc bit_ceil` signature `proc bit_ceil() -> int {` example `math.bit_ceil(...)` stability `stable`
- `proc bit_width` signature `proc bit_width() -> int {` example `math.bit_width(...)` stability `stable`
- `proc bit_count` signature `proc bit_count() -> int {` example `math.bit_count(...)` stability `stable`
- `proc lowbit` signature `proc lowbit() -> int {` example `math.lowbit(...)` stability `stable`
- `proc lowbit_pow2` signature `proc lowbit_pow2() -> int {` example `math.lowbit_pow2(...)` stability `stable`
- `proc parity` signature `proc parity() -> int {` example `math.parity(...)` stability `stable`
- `proc is_bit_set` signature `proc is_bit_set() -> int {` example `math.is_bit_set(...)` stability `stable`
- `proc set_bit` signature `proc set_bit() -> int {` example `math.set_bit(...)` stability `stable`
- `proc clear_bit` signature `proc clear_bit() -> int {` example `math.clear_bit(...)` stability `stable`
- `proc toggle_bit` signature `proc toggle_bit() -> int {` example `math.toggle_bit(...)` stability `stable`
- `proc align_down` signature `proc align_down() -> int {` example `math.align_down(...)` stability `stable`
- `proc align_up` signature `proc align_up() -> int {` example `math.align_up(...)` stability `stable`
- `proc round_to` signature `proc round_to() -> int {` example `math.round_to(...)` stability `stable`
- `proc floor_to` signature `proc floor_to() -> int {` example `math.floor_to(...)` stability `stable`
- `proc round_down_pow2` signature `proc round_down_pow2() -> int {` example `math.round_down_pow2(...)` stability `stable`
- `proc round_up_pow2` signature `proc round_up_pow2() -> int {` example `math.round_up_pow2(...)` stability `stable`
- `proc clamp_zero` signature `proc clamp_zero() -> int {` example `math.clamp_zero(...)` stability `stable`
- `proc clamp_nonzero` signature `proc clamp_nonzero() -> int {` example `math.clamp_nonzero(...)` stability `stable`
- `proc clamp_percent` signature `proc clamp_percent() -> int {` example `math.clamp_percent(...)` stability `stable`
- `proc lerp_int` signature `proc lerp_int() -> int {` example `math.lerp_int(...)` stability `stable`
- `proc inv_lerp_int` signature `proc inv_lerp_int() -> int {` example `math.inv_lerp_int(...)` stability `stable`
- `proc remap_int` signature `proc remap_int() -> int {` example `math.remap_int(...)` stability `stable`
- `proc smoothstep_scaled` signature `proc smoothstep_scaled() -> int {` example `math.smoothstep_scaled(...)` stability `stable`
- `proc ratio_new` signature `proc ratio_new() -> int {` example `math.ratio_new(...)` stability `stable`
- `proc ratio_add` signature `proc ratio_add() -> int {` example `math.ratio_add(...)` stability `stable`
- `proc ratio_sub` signature `proc ratio_sub() -> int {` example `math.ratio_sub(...)` stability `stable`
- `proc ratio_mul` signature `proc ratio_mul() -> int {` example `math.ratio_mul(...)` stability `stable`
- `proc ratio_div` signature `proc ratio_div() -> int {` example `math.ratio_div(...)` stability `stable`
- `proc ratio_to_scaled` signature `proc ratio_to_scaled() -> int {` example `math.ratio_to_scaled(...)` stability `stable`
- `proc ratio_percent` signature `proc ratio_percent() -> int {` example `math.ratio_percent(...)` stability `stable`
- `proc rect_area` signature `proc rect_area() -> int {` example `math.rect_area(...)` stability `stable`
- `proc rect_perimeter` signature `proc rect_perimeter() -> int {` example `math.rect_perimeter(...)` stability `stable`
- `proc triangle_area2` signature `proc triangle_area2() -> int {` example `math.triangle_area2(...)` stability `stable`
- `proc distance_sq` signature `proc distance_sq() -> int {` example `math.distance_sq(...)` stability `stable`
- `proc manhattan_distance` signature `proc manhattan_distance() -> int {` example `math.manhattan_distance(...)` stability `stable`
- `proc point` signature `proc point() -> int {` example `math.point(...)` stability `stable`
- `proc size2` signature `proc size2() -> int {` example `math.size2(...)` stability `stable`
- `proc rect` signature `proc rect() -> int {` example `math.rect(...)` stability `stable`
- `proc vec2i` signature `proc vec2i() -> int {` example `math.vec2i(...)` stability `stable`
- `proc vec3i` signature `proc vec3i() -> int {` example `math.vec3i(...)` stability `stable`
- `proc point_distance_sq` signature `proc point_distance_sq() -> int {` example `math.point_distance_sq(...)` stability `stable`
- `proc point_manhattan` signature `proc point_manhattan() -> int {` example `math.point_manhattan(...)` stability `stable`
- `proc rect_right` signature `proc rect_right() -> int {` example `math.rect_right(...)` stability `stable`
- `proc rect_bottom` signature `proc rect_bottom() -> int {` example `math.rect_bottom(...)` stability `stable`
- `proc rect_contains_point` signature `proc rect_contains_point() -> int {` example `math.rect_contains_point(...)` stability `stable`
- `proc rect_intersects` signature `proc rect_intersects() -> int {` example `math.rect_intersects(...)` stability `stable`
- `proc vec` signature `proc vec() -> int {` example `math.vec(...)` stability `stable`
- `proc vec_add` signature `proc vec_add() -> int {` example `math.vec_add(...)` stability `stable`
- `proc vec_sub` signature `proc vec_sub() -> int {` example `math.vec_sub(...)` stability `stable`
- `proc vec_scale` signature `proc vec_scale() -> int {` example `math.vec_scale(...)` stability `stable`
- `proc vec_dot` signature `proc vec_dot() -> int {` example `math.vec_dot(...)` stability `stable`
- `proc vec_norm_sq` signature `proc vec_norm_sq() -> int {` example `math.vec_norm_sq(...)` stability `stable`
- `proc vec2i_add` signature `proc vec2i_add() -> int {` example `math.vec2i_add(...)` stability `stable`
- `proc vec2i_sub` signature `proc vec2i_sub() -> int {` example `math.vec2i_sub(...)` stability `stable`
- `proc vec2i_scale` signature `proc vec2i_scale() -> int {` example `math.vec2i_scale(...)` stability `stable`
- `proc vec2i_dot` signature `proc vec2i_dot() -> int {` example `math.vec2i_dot(...)` stability `stable`
- `proc vec2i_cross` signature `proc vec2i_cross() -> int {` example `math.vec2i_cross(...)` stability `stable`
- `proc vec2i_norm_sq` signature `proc vec2i_norm_sq() -> int {` example `math.vec2i_norm_sq(...)` stability `stable`
- `proc vec3i_add` signature `proc vec3i_add() -> int {` example `math.vec3i_add(...)` stability `stable`
- `proc vec3i_sub` signature `proc vec3i_sub() -> int {` example `math.vec3i_sub(...)` stability `stable`
- `proc vec3i_scale` signature `proc vec3i_scale() -> int {` example `math.vec3i_scale(...)` stability `stable`
- `proc vec3i_dot` signature `proc vec3i_dot() -> int {` example `math.vec3i_dot(...)` stability `stable`
- `proc vec3i_cross` signature `proc vec3i_cross() -> int {` example `math.vec3i_cross(...)` stability `stable`
- `proc vec3i_norm_sq` signature `proc vec3i_norm_sq() -> int {` example `math.vec3i_norm_sq(...)` stability `stable`
- `proc cx` signature `proc cx() -> int {` example `math.cx(...)` stability `stable`
- `proc cx_add` signature `proc cx_add() -> int {` example `math.cx_add(...)` stability `stable`
- `proc cx_sub` signature `proc cx_sub() -> int {` example `math.cx_sub(...)` stability `stable`
- `proc cx_mul` signature `proc cx_mul() -> int {` example `math.cx_mul(...)` stability `stable`
- `proc cx_conj` signature `proc cx_conj() -> int {` example `math.cx_conj(...)` stability `stable`
- `proc cx_abs_sq` signature `proc cx_abs_sq() -> int {` example `math.cx_abs_sq(...)` stability `stable`
- `proc mat` signature `proc mat() -> int {` example `math.mat(...)` stability `stable`
- `proc mat_id` signature `proc mat_id() -> int {` example `math.mat_id(...)` stability `stable`
- `proc mat_add` signature `proc mat_add() -> int {` example `math.mat_add(...)` stability `stable`
- `proc mat_sub` signature `proc mat_sub() -> int {` example `math.mat_sub(...)` stability `stable`
- `proc mat_mul` signature `proc mat_mul() -> int {` example `math.mat_mul(...)` stability `stable`
- `proc mat_trace` signature `proc mat_trace() -> int {` example `math.mat_trace(...)` stability `stable`
- `proc mat_det` signature `proc mat_det() -> int {` example `math.mat_det(...)` stability `stable`
- `proc arr_len` signature `proc arr_len() -> int {` example `math.arr_len(...)` stability `stable`
- `proc arr_is_empty` signature `proc arr_is_empty() -> int {` example `math.arr_is_empty(...)` stability `stable`
- `proc arr_first` signature `proc arr_first() -> int {` example `math.arr_first(...)` stability `stable`
- `proc arr_last` signature `proc arr_last() -> int {` example `math.arr_last(...)` stability `stable`
- `proc arr_contains` signature `proc arr_contains() -> int {` example `math.arr_contains(...)` stability `stable`
- `proc arr_index_of` signature `proc arr_index_of() -> int {` example `math.arr_index_of(...)` stability `stable`
- `proc arr_count` signature `proc arr_count() -> int {` example `math.arr_count(...)` stability `stable`
- `proc arr_last_index_of` signature `proc arr_last_index_of() -> int {` example `math.arr_last_index_of(...)` stability `stable`
- `proc arr_copy` signature `proc arr_copy() -> int {` example `math.arr_copy(...)` stability `stable`
- `proc arr_append` signature `proc arr_append() -> int {` example `math.arr_append(...)` stability `stable`
- `proc arr_prepend` signature `proc arr_prepend() -> int {` example `math.arr_prepend(...)` stability `stable`
- `proc arr_concat` signature `proc arr_concat() -> int {` example `math.arr_concat(...)` stability `stable`
- `proc arr_push` signature `proc arr_push() -> int {` example `math.arr_push(...)` stability `stable`
- `proc arr_set` signature `proc arr_set() -> int {` example `math.arr_set(...)` stability `stable`
- `proc arr_swap` signature `proc arr_swap() -> int {` example `math.arr_swap(...)` stability `stable`
- `proc arr_clear` signature `proc arr_clear() -> int {` example `math.arr_clear(...)` stability `stable`
- `proc arr_pop` signature `proc arr_pop() -> int {` example `math.arr_pop(...)` stability `stable`
- `proc arr_pop_at` signature `proc arr_pop_at() -> int {` example `math.arr_pop_at(...)` stability `stable`
- `proc arr_insert` signature `proc arr_insert() -> int {` example `math.arr_insert(...)` stability `stable`
- `proc arr_remove_at` signature `proc arr_remove_at() -> int {` example `math.arr_remove_at(...)` stability `stable`
- `proc arr_repeat` signature `proc arr_repeat() -> int {` example `math.arr_repeat(...)` stability `stable`
- `proc arr_fill` signature `proc arr_fill() -> int {` example `math.arr_fill(...)` stability `stable`
- `proc arr_reverse` signature `proc arr_reverse() -> int {` example `math.arr_reverse(...)` stability `stable`
- `proc arr_sum` signature `proc arr_sum() -> int {` example `math.arr_sum(...)` stability `stable`
- `proc arr_min` signature `proc arr_min() -> int {` example `math.arr_min(...)` stability `stable`
- `proc arr_max` signature `proc arr_max() -> int {` example `math.arr_max(...)` stability `stable`
- `proc arr_mean` signature `proc arr_mean() -> int {` example `math.arr_mean(...)` stability `stable`
- `proc arr_mean_scaled` signature `proc arr_mean_scaled() -> int {` example `math.arr_mean_scaled(...)` stability `stable`
- `proc arr_range` signature `proc arr_range() -> int {` example `math.arr_range(...)` stability `stable`
- `proc arr_sum_abs` signature `proc arr_sum_abs() -> int {` example `math.arr_sum_abs(...)` stability `stable`
- `proc arr_product` signature `proc arr_product() -> int {` example `math.arr_product(...)` stability `stable`
- `proc arr_any` signature `proc arr_any() -> int {` example `math.arr_any(...)` stability `stable`
- `proc arr_all` signature `proc arr_all() -> int {` example `math.arr_all(...)` stability `stable`
- `proc arr_count_nonzero` signature `proc arr_count_nonzero() -> int {` example `math.arr_count_nonzero(...)` stability `stable`
- `proc arr_prefix_sum` signature `proc arr_prefix_sum() -> int {` example `math.arr_prefix_sum(...)` stability `stable`
- `proc arr_prefix_min` signature `proc arr_prefix_min() -> int {` example `math.arr_prefix_min(...)` stability `stable`
- `proc arr_prefix_max` signature `proc arr_prefix_max() -> int {` example `math.arr_prefix_max(...)` stability `stable`
- `proc arr_take` signature `proc arr_take() -> int {` example `math.arr_take(...)` stability `stable`
- `proc arr_drop` signature `proc arr_drop() -> int {` example `math.arr_drop(...)` stability `stable`
- `proc arr_head` signature `proc arr_head() -> int {` example `math.arr_head(...)` stability `stable`
- `proc arr_tail` signature `proc arr_tail() -> int {` example `math.arr_tail(...)` stability `stable`
- `proc arr_slice` signature `proc arr_slice() -> int {` example `math.arr_slice(...)` stability `stable`
- `proc arr_rotate_left` signature `proc arr_rotate_left() -> int {` example `math.arr_rotate_left(...)` stability `stable`
- `proc arr_rotate_right` signature `proc arr_rotate_right() -> int {` example `math.arr_rotate_right(...)` stability `stable`
- `proc arr_replace_slice` signature `proc arr_replace_slice() -> int {` example `math.arr_replace_slice(...)` stability `stable`
- `proc arr_splice` signature `proc arr_splice() -> int {` example `math.arr_splice(...)` stability `stable`
- `proc arr_rotate` signature `proc arr_rotate() -> int {` example `math.arr_rotate(...)` stability `stable`
- `proc arr_enumerate` signature `proc arr_enumerate() -> int {` example `math.arr_enumerate(...)` stability `stable`
- `proc arr_zip` signature `proc arr_zip() -> int {` example `math.arr_zip(...)` stability `stable`
- `proc arr_chunks` signature `proc arr_chunks() -> int {` example `math.arr_chunks(...)` stability `stable`
- `proc arr_clamp_each` signature `proc arr_clamp_each() -> int {` example `math.arr_clamp_each(...)` stability `stable`
- `proc arr_get` signature `proc arr_get() -> int {` example `math.arr_get(...)` stability `stable`
- `proc arr_stats` signature `proc arr_stats() -> int {` example `math.arr_stats(...)` stability `stable`
- `proc arr_sort_simple` signature `proc arr_sort_simple() -> int {` example `math.arr_sort_simple(...)` stability `stable`
- `proc arr_median_floor` signature `proc arr_median_floor() -> int {` example `math.arr_median_floor(...)` stability `stable`
- `proc arr_variance_scaled` signature `proc arr_variance_scaled() -> int {` example `math.arr_variance_scaled(...)` stability `stable`
- `proc arr_stddev_floor` signature `proc arr_stddev_floor() -> int {` example `math.arr_stddev_floor(...)` stability `stable`
- `proc permutations` signature `proc permutations() -> int {` example `math.permutations(...)` stability `stable`
- `proc combinations` signature `proc combinations() -> int {` example `math.combinations(...)` stability `stable`
- `proc probability_percent` signature `proc probability_percent() -> int {` example `math.probability_percent(...)` stability `stable`
- `proc odds_ratio` signature `proc odds_ratio() -> int {` example `math.odds_ratio(...)` stability `stable`
- `proc pairings` signature `proc pairings() -> int {` example `math.pairings(...)` stability `stable`
- `proc percent` signature `proc percent() -> int {` example `math.percent(...)` stability `stable`
- `proc percent_scaled` signature `proc percent_scaled() -> int {` example `math.percent_scaled(...)` stability `stable`
- `proc arithmetic_term` signature `proc arithmetic_term() -> int {` example `math.arithmetic_term(...)` stability `stable`
- `proc arithmetic_sum` signature `proc arithmetic_sum() -> int {` example `math.arithmetic_sum(...)` stability `stable`
- `proc geometric_term` signature `proc geometric_term() -> int {` example `math.geometric_term(...)` stability `stable`
- `proc triangular_term` signature `proc triangular_term() -> int {` example `math.triangular_term(...)` stability `stable`
- `proc fibonacci_term` signature `proc fibonacci_term() -> int {` example `math.fibonacci_term(...)` stability `stable`
- `proc finite_difference` signature `proc finite_difference() -> int {` example `math.finite_difference(...)` stability `stable`
- `proc slope_between` signature `proc slope_between() -> int {` example `math.slope_between(...)` stability `stable`
- `proc derivative_at_int` signature `proc derivative_at_int() -> int {` example `math.derivative_at_int(...)` stability `stable`
- `proc normalize_degrees` signature `proc normalize_degrees() -> int {` example `math.normalize_degrees(...)` stability `stable`
- `proc opposite_angle` signature `proc opposite_angle() -> int {` example `math.opposite_angle(...)` stability `stable`
- `proc complement_angle` signature `proc complement_angle() -> int {` example `math.complement_angle(...)` stability `stable`
- `proc supplement_angle` signature `proc supplement_angle() -> int {` example `math.supplement_angle(...)` stability `stable`
- `proc quadrant` signature `proc quadrant() -> int {` example `math.quadrant(...)` stability `stable`
- `proc is_right_angle` signature `proc is_right_angle() -> int {` example `math.is_right_angle(...)` stability `stable`
- `proc is_straight_angle` signature `proc is_straight_angle() -> int {` example `math.is_straight_angle(...)` stability `stable`
- `proc degrees_to_radians` signature `proc degrees_to_radians() -> int {` example `math.degrees_to_radians(...)` stability `stable`
- `proc radians_to_degrees` signature `proc radians_to_degrees() -> int {` example `math.radians_to_degrees(...)` stability `stable`
- `proc sin` signature `proc sin() -> int {` example `math.sin(...)` stability `stable`
- `proc cos` signature `proc cos() -> int {` example `math.cos(...)` stability `stable`
- `proc tan` signature `proc tan() -> int {` example `math.tan(...)` stability `stable`
- `proc asin` signature `proc asin() -> int {` example `math.asin(...)` stability `stable`
- `proc acos` signature `proc acos() -> int {` example `math.acos(...)` stability `stable`
- `proc atan` signature `proc atan() -> int {` example `math.atan(...)` stability `stable`
- `proc atan2` signature `proc atan2() -> int {` example `math.atan2(...)` stability `stable`
- `proc sinh` signature `proc sinh() -> int {` example `math.sinh(...)` stability `stable`
- `proc cosh` signature `proc cosh() -> int {` example `math.cosh(...)` stability `stable`
- `proc tanh` signature `proc tanh() -> int {` example `math.tanh(...)` stability `stable`
- `proc asinh` signature `proc asinh() -> int {` example `math.asinh(...)` stability `stable`
- `proc acosh` signature `proc acosh() -> int {` example `math.acosh(...)` stability `stable`
- `proc atanh` signature `proc atanh() -> int {` example `math.atanh(...)` stability `stable`
- `proc exp` signature `proc exp() -> int {` example `math.exp(...)` stability `stable`
- `proc log` signature `proc log() -> int {` example `math.log(...)` stability `stable`
- `proc log10` signature `proc log10() -> int {` example `math.log10(...)` stability `stable`
- `proc pow` signature `proc pow() -> int {` example `math.pow(...)` stability `stable`
- `proc sqrt` signature `proc sqrt() -> int {` example `math.sqrt(...)` stability `stable`
- `proc fabs` signature `proc fabs() -> int {` example `math.fabs(...)` stability `stable`
- `proc fmod` signature `proc fmod() -> int {` example `math.fmod(...)` stability `stable`
- `proc ceil` signature `proc ceil() -> int {` example `math.ceil(...)` stability `stable`
- `proc floor` signature `proc floor() -> int {` example `math.floor(...)` stability `stable`
- `proc round_f64` signature `proc round_f64() -> int {` example `math.round_f64(...)` stability `stable`
- `proc trunc_f64` signature `proc trunc_f64() -> int {` example `math.trunc_f64(...)` stability `stable`
- `proc fract_f64` signature `proc fract_f64() -> int {` example `math.fract_f64(...)` stability `stable`
- `proc clamp_f64` signature `proc clamp_f64() -> int {` example `math.clamp_f64(...)` stability `stable`
- `proc min_f64` signature `proc min_f64() -> int {` example `math.min_f64(...)` stability `stable`
- `proc max_f64` signature `proc max_f64() -> int {` example `math.max_f64(...)` stability `stable`
- `proc lerp_f64` signature `proc lerp_f64() -> int {` example `math.lerp_f64(...)` stability `stable`
- `proc smoothstep_f64` signature `proc smoothstep_f64() -> int {` example `math.smoothstep_f64(...)` stability `stable`
- `proc is_nan` signature `proc is_nan() -> int {` example `math.is_nan(...)` stability `stable`
- `proc is_infinite` signature `proc is_infinite() -> int {` example `math.is_infinite(...)` stability `stable`
- `proc is_finite` signature `proc is_finite() -> int {` example `math.is_finite(...)` stability `stable`
- `proc math_ready` signature `proc math_ready() -> int {` example `math.math_ready(...)` stability `stable`
- `proc math_health` signature `proc math_health() -> int {` example `math.math_health(...)` stability `stable`
- `proc math_summary` signature `proc math_summary() -> int {` example `math.math_summary(...)` stability `stable`
- `proc math_domains` signature `proc math_domains() -> int {` example `math.math_domains(...)` stability `stable`
- `proc library_meta` signature `proc library_meta() -> int {` example `math.library_meta(...)` stability `stable`
- `proc math_selftest` signature `proc math_selftest() -> int {` example `math.math_selftest(...)` stability `stable`
- `proc stdlib_smoke_1` signature `proc stdlib_smoke_1() -> int {` example `math.stdlib_smoke_1(...)` stability `stable`
- `proc stdlib_smoke_2` signature `proc stdlib_smoke_2() -> int {` example `math.stdlib_smoke_2(...)` stability `stable`
- `proc stdlib_smoke_3` signature `proc stdlib_smoke_3() -> int {` example `math.stdlib_smoke_3(...)` stability `stable`
- `proc stdlib_smoke_4` signature `proc stdlib_smoke_4() -> int {` example `math.stdlib_smoke_4(...)` stability `stable`
- `proc stdlib_smoke_5` signature `proc stdlib_smoke_5() -> int {` example `math.stdlib_smoke_5(...)` stability `stable`

## `src/vitte/stdlib/strings.vitl`

Stability: `stable`

- `form StringLibraryManifest` signature `form StringLibraryManifest {` example `strings.StringLibraryManifest` stability `stable`
- `form StringLibraryHealth` signature `form StringLibraryHealth {` example `strings.StringLibraryHealth` stability `stable`
- `form StringLibrarySummary` signature `form StringLibrarySummary {` example `strings.StringLibrarySummary` stability `stable`
- `form StringBuilder` signature `form StringBuilder {` example `strings.StringBuilder` stability `stable`
- `form StringBuilderReport` signature `form StringBuilderReport {` example `strings.StringBuilderReport` stability `stable`
- `proc sb_new` signature `proc sb_new(capacity: i32) -> StringBuilder {` example `strings.sb_new(...)` stability `stable`
- `proc sb_append` signature `proc sb_append(sb: StringBuilder, text_value: string) -> int {` example `strings.sb_append(...)` stability `stable`
- `proc sb_append_char` signature `proc sb_append_char(sb: StringBuilder, ch: i32) -> int {` example `strings.sb_append_char(...)` stability `stable`
- `proc sb_append_int` signature `proc sb_append_int(sb: StringBuilder, value: i32) -> int {` example `strings.sb_append_int(...)` stability `stable`
- `proc sb_insert` signature `proc sb_insert(sb: StringBuilder, index: i32, text_value: string) -> int {` example `strings.sb_insert(...)` stability `stable`
- `proc sb_delete` signature `proc sb_delete(sb: StringBuilder, start: i32, end: i32) -> int {` example `strings.sb_delete(...)` stability `stable`
- `proc sb_replace` signature `proc sb_replace(sb: StringBuilder, oldstr: string, newstr: string) -> int {` example `strings.sb_replace(...)` stability `stable`
- `proc sb_to_string` signature `proc sb_to_string(sb: StringBuilder) -> string {` example `strings.sb_to_string(...)` stability `stable`
- `proc sb_clear` signature `proc sb_clear(sb: StringBuilder) {` example `strings.sb_clear(...)` stability `stable`
- `proc sb_length` signature `proc sb_length(sb: StringBuilder) -> i32 {` example `strings.sb_length(...)` stability `stable`
- `proc sb_version` signature `proc sb_version() -> string {` example `strings.sb_version(...)` stability `stable`
- `proc sb_ready` signature `proc sb_ready() -> bool {` example `strings.sb_ready(...)` stability `stable`
- `proc sb_report` signature `proc sb_report(sb: StringBuilder) -> StringBuilderReport {` example `strings.sb_report(...)` stability `stable`
- `proc sb_selftest` signature `proc sb_selftest() -> bool {` example `strings.sb_selftest(...)` stability `stable`
- `proc strings_version` signature `proc strings_version() -> string {` example `strings.strings_version(...)` stability `stable`
- `proc strings_name` signature `proc strings_name() -> string {` example `strings.strings_name(...)` stability `stable`
- `proc strings_module_count` signature `proc strings_module_count() -> i32 {` example `strings.strings_module_count(...)` stability `stable`
- `proc strings_modules` signature `proc strings_modules() -> [string] {` example `strings.strings_modules(...)` stability `stable`
- `proc strings_manifest` signature `proc strings_manifest() -> StringLibraryManifest {` example `strings.strings_manifest(...)` stability `stable`
- `proc strings_ready` signature `proc strings_ready() -> bool {` example `strings.strings_ready(...)` stability `stable`
- `proc strings_health` signature `proc strings_health() -> StringLibraryHealth {` example `strings.strings_health(...)` stability `stable`
- `proc strings_summary` signature `proc strings_summary() -> StringLibrarySummary {` example `strings.strings_summary(...)` stability `stable`
- `proc strings_selftest` signature `proc strings_selftest() -> bool {` example `strings.strings_selftest(...)` stability `stable`
- `form StringNormalizeReport` signature `form StringNormalizeReport {` example `strings.StringNormalizeReport` stability `stable`
- `form StringFormatReport` signature `form StringFormatReport {` example `strings.StringFormatReport` stability `stable`
- `form StringSearchReport` signature `form StringSearchReport {` example `strings.StringSearchReport` stability `stable`
- `form StringTokenizeReport` signature `form StringTokenizeReport {` example `strings.StringTokenizeReport` stability `stable`
- `form StringUnicodeReport` signature `form StringUnicodeReport {` example `strings.StringUnicodeReport` stability `stable`
- `form StringMetricsReport` signature `form StringMetricsReport {` example `strings.StringMetricsReport` stability `stable`
- `form StringTemplateReport` signature `form StringTemplateReport {` example `strings.StringTemplateReport` stability `stable`
- `form StringPredicateReport` signature `form StringPredicateReport {` example `strings.StringPredicateReport` stability `stable`
- `form StringTransformReport` signature `form StringTransformReport {` example `strings.StringTransformReport` stability `stable`
- `form StringAsciiReport` signature `form StringAsciiReport {` example `strings.StringAsciiReport` stability `stable`
- `form StringWhitespaceReport` signature `form StringWhitespaceReport {` example `strings.StringWhitespaceReport` stability `stable`
- `form StringLocaleReport` signature `form StringLocaleReport {` example `strings.StringLocaleReport` stability `stable`
- `form StringPatternReport` signature `form StringPatternReport {` example `strings.StringPatternReport` stability `stable`
- `form StringInflectionReport` signature `form StringInflectionReport {` example `strings.StringInflectionReport` stability `stable`
- `proc str_starts_with` signature `proc str_starts_with(text_value: string, prefix: string) -> int {` example `strings.str_starts_with(...)` stability `stable`
- `proc str_ends_with` signature `proc str_ends_with(text_value: string, suffix: string) -> int {` example `strings.str_ends_with(...)` stability `stable`
- `proc str_contains` signature `proc str_contains(text_value: string, substr: string) -> int {` example `strings.str_contains(...)` stability `stable`
- `proc str_index_of` signature `proc str_index_of(text_value: string, substr: string) -> i32 {` example `strings.str_index_of(...)` stability `stable`
- `proc str_last_index_of` signature `proc str_last_index_of(text_value: string, substr: string) -> i32 {` example `strings.str_last_index_of(...)` stability `stable`
- `proc str_substring` signature `proc str_substring(text_value: string, start: i32, end: i32) -> string {` example `strings.str_substring(...)` stability `stable`
- `proc str_split` signature `proc str_split(text_value: string, delimiter: string) -> [string] {` example `strings.str_split(...)` stability `stable`
- `proc str_split_limit` signature `proc str_split_limit(text_value: string, delimiter: string, limit: i32) -> [string] {` example `strings.str_split_limit(...)` stability `stable`
- `proc str_join` signature `proc str_join(parts: [string], delimiter: string) -> string {` example `strings.str_join(...)` stability `stable`
- `proc str_repeat` signature `proc str_repeat(text_value: string, count: i32) -> string {` example `strings.str_repeat(...)` stability `stable`
- `proc str_reverse` signature `proc str_reverse(text_value: string) -> string {` example `strings.str_reverse(...)` stability `stable`
- `proc str_pad_left` signature `proc str_pad_left(text_value: string, width: i32, padchar: i32) -> string {` example `strings.str_pad_left(...)` stability `stable`
- `proc str_pad_right` signature `proc str_pad_right(text_value: string, width: i32, padchar: i32) -> string {` example `strings.str_pad_right(...)` stability `stable`
- `proc str_center` signature `proc str_center(text_value: string, width: i32, padchar: i32) -> string {` example `strings.str_center(...)` stability `stable`
- `proc str_indent` signature `proc str_indent(text_value: string, spaces: i32) -> string {` example `strings.str_indent(...)` stability `stable`
- `proc str_dedent` signature `proc str_dedent(text_value: string) -> string {` example `strings.str_dedent(...)` stability `stable`
- `proc str_uppercase` signature `proc str_uppercase(text_value: string) -> string {` example `strings.str_uppercase(...)` stability `stable`
- `proc str_lowercase` signature `proc str_lowercase(text_value: string) -> string {` example `strings.str_lowercase(...)` stability `stable`
- `proc str_title_case` signature `proc str_title_case(text_value: string) -> string {` example `strings.str_title_case(...)` stability `stable`
- `proc str_capitalize` signature `proc str_capitalize(text_value: string) -> string {` example `strings.str_capitalize(...)` stability `stable`
- `proc str_swap_case` signature `proc str_swap_case(text_value: string) -> string {` example `strings.str_swap_case(...)` stability `stable`
- `proc str_trim` signature `proc str_trim(text_value: string) -> string {` example `strings.str_trim(...)` stability `stable`
- `proc str_trim_left` signature `proc str_trim_left(text_value: string) -> string {` example `strings.str_trim_left(...)` stability `stable`
- `proc str_trim_right` signature `proc str_trim_right(text_value: string) -> string {` example `strings.str_trim_right(...)` stability `stable`
- `proc str_trim_chars` signature `proc str_trim_chars(text_value: string, chars: string) -> string {` example `strings.str_trim_chars(...)` stability `stable`
- `proc str_equals` signature `proc str_equals(str1: string, str2: string) -> int {` example `strings.str_equals(...)` stability `stable`
- `proc str_equals_ignore_case` signature `proc str_equals_ignore_case(str1: string, str2: string) -> int {` example `strings.str_equals_ignore_case(...)` stability `stable`
- `proc str_compare` signature `proc str_compare(str1: string, str2: string) -> int {` example `strings.str_compare(...)` stability `stable`
- `proc str_levenshtein_distance` signature `proc str_levenshtein_distance(str1: string, str2: string) -> i32 {` example `strings.str_levenshtein_distance(...)` stability `stable`
- `proc str_escape` signature `proc str_escape(text_value: string) -> string {` example `strings.str_escape(...)` stability `stable`
- `proc str_unescape` signature `proc str_unescape(text_value: string) -> string {` example `strings.str_unescape(...)` stability `stable`
- `proc str_escape_quotes` signature `proc str_escape_quotes(text_value: string) -> string {` example `strings.str_escape_quotes(...)` stability `stable`
- `proc str_escape_newlines` signature `proc str_escape_newlines(text_value: string) -> string {` example `strings.str_escape_newlines(...)` stability `stable`
- `proc str_format` signature `proc str_format(template: string, args: [string]) -> string {` example `strings.str_format(...)` stability `stable`
- `proc str_interpolate` signature `proc str_interpolate(template: string) -> string {` example `strings.str_interpolate(...)` stability `stable`
- `proc is_digit` signature `proc is_digit(ch: i32) -> int {` example `strings.is_digit(...)` stability `stable`
- `proc is_alpha` signature `proc is_alpha(ch: i32) -> int {` example `strings.is_alpha(...)` stability `stable`
- `proc is_alnum` signature `proc is_alnum(ch: i32) -> int {` example `strings.is_alnum(...)` stability `stable`
- `proc is_space` signature `proc is_space(ch: i32) -> int {` example `strings.is_space(...)` stability `stable`
- `proc is_upper` signature `proc is_upper(ch: i32) -> int {` example `strings.is_upper(...)` stability `stable`
- `proc is_lower` signature `proc is_lower(ch: i32) -> int {` example `strings.is_lower(...)` stability `stable`
- `proc is_punct` signature `proc is_punct(ch: i32) -> int {` example `strings.is_punct(...)` stability `stable`
- `proc str_replace` signature `proc str_replace(text_value: string, oldstr: string, newstr: string) -> string {` example `strings.str_replace(...)` stability `stable`
- `proc str_replace_all` signature `proc str_replace_all(text_value: string, oldstr: string, newstr: string) -> string {` example `strings.str_replace_all(...)` stability `stable`
- `proc str_remove_prefix` signature `proc str_remove_prefix(text_value: string, prefix: string) -> string {` example `strings.str_remove_prefix(...)` stability `stable`
- `proc str_remove_suffix` signature `proc str_remove_suffix(text_value: string, suffix: string) -> string {` example `strings.str_remove_suffix(...)` stability `stable`
- `proc str_normalize_whitespace` signature `proc str_normalize_whitespace(text_value: string) -> string {` example `strings.str_normalize_whitespace(...)` stability `stable`
- `proc str_slug` signature `proc str_slug(text_value: string) -> string {` example `strings.str_slug(...)` stability `stable`
- `proc str_snake_case` signature `proc str_snake_case(text_value: string) -> string {` example `strings.str_snake_case(...)` stability `stable`
- `proc str_kebab_case` signature `proc str_kebab_case(text_value: string) -> string {` example `strings.str_kebab_case(...)` stability `stable`
- `proc str_camel_case` signature `proc str_camel_case(text_value: string) -> string {` example `strings.str_camel_case(...)` stability `stable`
- `proc str_pascal_case` signature `proc str_pascal_case(text_value: string) -> string {` example `strings.str_pascal_case(...)` stability `stable`
- `proc str_wrap` signature `proc str_wrap(text_value: string, width: i32) -> [string] {` example `strings.str_wrap(...)` stability `stable`
- `proc str_lines` signature `proc str_lines(text_value: string) -> [string] {` example `strings.str_lines(...)` stability `stable`
- `proc str_words` signature `proc str_words(text_value: string) -> [string] {` example `strings.str_words(...)` stability `stable`
- `proc str_tokens` signature `proc str_tokens(text_value: string) -> [string] {` example `strings.str_tokens(...)` stability `stable`
- `proc str_count` signature `proc str_count(text_value: string, needle: string) -> i32 {` example `strings.str_count(...)` stability `stable`
- `proc str_is_empty` signature `proc str_is_empty(text_value: string) -> int {` example `strings.str_is_empty(...)` stability `stable`
- `proc str_is_blank` signature `proc str_is_blank(text_value: string) -> int {` example `strings.str_is_blank(...)` stability `stable`
- `proc str_ascii_only` signature `proc str_ascii_only(text_value: string) -> int {` example `strings.str_ascii_only(...)` stability `stable`
- `proc str_repeat_char` signature `proc str_repeat_char(ch: i32, count: i32) -> string {` example `strings.str_repeat_char(...)` stability `stable`
- `proc str_center_text` signature `proc str_center_text(text_value: string, width: i32) -> string {` example `strings.str_center_text(...)` stability `stable`
- `proc str_truncate` signature `proc str_truncate(text_value: string, width: i32) -> string {` example `strings.str_truncate(...)` stability `stable`
- `proc str_head` signature `proc str_head(text_value: string, count: i32) -> string {` example `strings.str_head(...)` stability `stable`
- `proc str_tail` signature `proc str_tail(text_value: string, count: i32) -> string {` example `strings.str_tail(...)` stability `stable`
- `proc str_normalize_report` signature `proc str_normalize_report(text_value: string) -> StringNormalizeReport {` example `strings.str_normalize_report(...)` stability `stable`
- `proc str_format_report` signature `proc str_format_report(template: string) -> StringFormatReport {` example `strings.str_format_report(...)` stability `stable`
- `proc str_search_report` signature `proc str_search_report(haystack: string, needle: string) -> StringSearchReport {` example `strings.str_search_report(...)` stability `stable`
- `proc str_tokenize_report` signature `proc str_tokenize_report(text_value: string) -> StringTokenizeReport {` example `strings.str_tokenize_report(...)` stability `stable`
- `proc strings_max_report` signature `proc strings_max_report() -> StringLibrarySummary {` example `strings.strings_max_report(...)` stability `stable`
- `proc str_ascii_strip` signature `proc str_ascii_strip(text_value: string) -> string {` example `strings.str_ascii_strip(...)` stability `stable`
- `proc str_ascii_only_text` signature `proc str_ascii_only_text(text_value: string) -> string {` example `strings.str_ascii_only_text(...)` stability `stable`
- `proc str_whitespace_collapse` signature `proc str_whitespace_collapse(text_value: string) -> string {` example `strings.str_whitespace_collapse(...)` stability `stable`
- `proc str_whitespace_trim_lines` signature `proc str_whitespace_trim_lines(text_value: string) -> string {` example `strings.str_whitespace_trim_lines(...)` stability `stable`
- `proc str_locale_normalize` signature `proc str_locale_normalize(text_value: string) -> string {` example `strings.str_locale_normalize(...)` stability `stable`
- `proc str_locale_compare` signature `proc str_locale_compare(str1: string, str2: string) -> i32 {` example `strings.str_locale_compare(...)` stability `stable`
- `proc str_pattern_match` signature `proc str_pattern_match(text_value: string, pattern: string) -> int {` example `strings.str_pattern_match(...)` stability `stable`
- `proc str_pattern_replace` signature `proc str_pattern_replace(text_value: string, pattern: string, replacement: string) -> string {` example `strings.str_pattern_replace(...)` stability `stable`
- `proc str_pattern_find` signature `proc str_pattern_find(text_value: string, pattern: string) -> i32 {` example `strings.str_pattern_find(...)` stability `stable`
- `proc str_pluralize` signature `proc str_pluralize(text_value: string) -> string {` example `strings.str_pluralize(...)` stability `stable`
- `proc str_singularize` signature `proc str_singularize(text_value: string) -> string {` example `strings.str_singularize(...)` stability `stable`
- `proc str_inflect_count` signature `proc str_inflect_count(word: string, count: i32) -> string {` example `strings.str_inflect_count(...)` stability `stable`
- `proc str_ascii_report` signature `proc str_ascii_report(text_value: string) -> StringAsciiReport {` example `strings.str_ascii_report(...)` stability `stable`
- `proc str_whitespace_report` signature `proc str_whitespace_report(text_value: string) -> StringWhitespaceReport {` example `strings.str_whitespace_report(...)` stability `stable`
- `proc str_locale_report` signature `proc str_locale_report(text_value: string) -> StringLocaleReport {` example `strings.str_locale_report(...)` stability `stable`
- `proc str_pattern_report` signature `proc str_pattern_report(text_value: string, pattern: string) -> StringPatternReport {` example `strings.str_pattern_report(...)` stability `stable`
- `proc str_inflection_report` signature `proc str_inflection_report(text_value: string) -> StringInflectionReport {` example `strings.str_inflection_report(...)` stability `stable`
- `proc str_unicode_normalize` signature `proc str_unicode_normalize(text_value: string) -> string {` example `strings.str_unicode_normalize(...)` stability `stable`
- `proc str_unicode_fold` signature `proc str_unicode_fold(text_value: string) -> string {` example `strings.str_unicode_fold(...)` stability `stable`
- `proc str_unicode_strip_accents` signature `proc str_unicode_strip_accents(text_value: string) -> string {` example `strings.str_unicode_strip_accents(...)` stability `stable`
- `proc str_length` signature `proc str_length(text_value: string) -> i32 {` example `strings.str_length(...)` stability `stable`
- `proc str_word_count` signature `proc str_word_count(text_value: string) -> i32 {` example `strings.str_word_count(...)` stability `stable`
- `proc str_line_count` signature `proc str_line_count(text_value: string) -> i32 {` example `strings.str_line_count(...)` stability `stable`
- `proc str_template_apply` signature `proc str_template_apply(template: string, data: [string]) -> string {` example `strings.str_template_apply(...)` stability `stable`
- `proc str_template_render` signature `proc str_template_render(template: string) -> string {` example `strings.str_template_render(...)` stability `stable`
- `proc str_template_expand` signature `proc str_template_expand(template: string) -> string {` example `strings.str_template_expand(...)` stability `stable`
- `proc str_is_empty_report` signature `proc str_is_empty_report(text_value: string) -> StringPredicateReport {` example `strings.str_is_empty_report(...)` stability `stable`
- `proc str_transform_upper` signature `proc str_transform_upper(text_value: string) -> string {` example `strings.str_transform_upper(...)` stability `stable`
- `proc str_transform_lower` signature `proc str_transform_lower(text_value: string) -> string {` example `strings.str_transform_lower(...)` stability `stable`
- `proc str_transform_title` signature `proc str_transform_title(text_value: string) -> string {` example `strings.str_transform_title(...)` stability `stable`
- `proc str_transform_slug` signature `proc str_transform_slug(text_value: string) -> string {` example `strings.str_transform_slug(...)` stability `stable`
- `proc str_transform_snake` signature `proc str_transform_snake(text_value: string) -> string {` example `strings.str_transform_snake(...)` stability `stable`
- `proc str_transform_kebab` signature `proc str_transform_kebab(text_value: string) -> string {` example `strings.str_transform_kebab(...)` stability `stable`
- `proc str_transform_report` signature `proc str_transform_report(text_value: string) -> StringTransformReport {` example `strings.str_transform_report(...)` stability `stable`
- `proc string_report` signature `proc string_report() -> StringLibrarySummary {` example `strings.string_report(...)` stability `stable`

## `src/vitte/stdlib/encoding.vitl`

Stability: `stable`

- `proc base64_encode` signature `proc base64_encode(data: string) -> string {` example `encoding.base64_encode(...)` stability `stable`
- `proc base64_decode` signature `proc base64_decode(data: string) -> string {` example `encoding.base64_decode(...)` stability `stable`
- `proc base64_encode_url_safe` signature `proc base64_encode_url_safe(data: string) -> string {` example `encoding.base64_encode_url_safe(...)` stability `stable`
- `proc base64_decode_url_safe` signature `proc base64_decode_url_safe(data: string) -> string {` example `encoding.base64_decode_url_safe(...)` stability `stable`
- `proc hex_encode` signature `proc hex_encode(data: string) -> string {` example `encoding.hex_encode(...)` stability `stable`
- `proc hex_decode` signature `proc hex_decode(data: string) -> string {` example `encoding.hex_decode(...)` stability `stable`
- `proc hex_encode_uppercase` signature `proc hex_encode_uppercase(data: string) -> string {` example `encoding.hex_encode_uppercase(...)` stability `stable`
- `proc hex_encode_lowercase` signature `proc hex_encode_lowercase(data: string) -> string {` example `encoding.hex_encode_lowercase(...)` stability `stable`
- `proc url_encode` signature `proc url_encode(text_value: string) -> string {` example `encoding.url_encode(...)` stability `stable`
- `proc url_decode` signature `proc url_decode(text_value: string) -> string {` example `encoding.url_decode(...)` stability `stable`
- `proc url_encode_component` signature `proc url_encode_component(text_value: string) -> string {` example `encoding.url_encode_component(...)` stability `stable`
- `proc url_decode_component` signature `proc url_decode_component(text_value: string) -> string {` example `encoding.url_decode_component(...)` stability `stable`
- `proc url_query_escape` signature `proc url_query_escape(text_value: string) -> string {` example `encoding.url_query_escape(...)` stability `stable`
- `proc url_query_unescape` signature `proc url_query_unescape(text_value: string) -> string {` example `encoding.url_query_unescape(...)` stability `stable`
- `proc html_escape` signature `proc html_escape(text_value: string) -> string {` example `encoding.html_escape(...)` stability `stable`
- `proc html_unescape` signature `proc html_unescape(text_value: string) -> string {` example `encoding.html_unescape(...)` stability `stable`
- `proc xml_escape` signature `proc xml_escape(text_value: string) -> string {` example `encoding.xml_escape(...)` stability `stable`
- `proc xml_unescape` signature `proc xml_unescape(text_value: string) -> string {` example `encoding.xml_unescape(...)` stability `stable`
- `proc cdata_escape` signature `proc cdata_escape(text_value: string) -> string {` example `encoding.cdata_escape(...)` stability `stable`
- `proc utf8_encode` signature `proc utf8_encode(code_point: i32) -> string {` example `encoding.utf8_encode(...)` stability `stable`
- `proc utf8_decode` signature `proc utf8_decode(encoded: string) -> i32 {` example `encoding.utf8_decode(...)` stability `stable`
- `proc utf8_is_valid` signature `proc utf8_is_valid(text_value: string) -> int {` example `encoding.utf8_is_valid(...)` stability `stable`
- `proc utf8_length` signature `proc utf8_length(text_value: string) -> i32 {` example `encoding.utf8_length(...)` stability `stable`
- `proc utf8_byte_length` signature `proc utf8_byte_length(text_value: string) -> i32 {` example `encoding.utf8_byte_length(...)` stability `stable`
- `proc utf8_char_at` signature `proc utf8_char_at(text_value: string, index: i32) -> i32 {` example `encoding.utf8_char_at(...)` stability `stable`
- `proc utf8_substring` signature `proc utf8_substring(text_value: string, start: i32, end: i32) -> string {` example `encoding.utf8_substring(...)` stability `stable`
- `proc utf16_encode` signature `proc utf16_encode(code_point: i32) -> string {` example `encoding.utf16_encode(...)` stability `stable`
- `proc utf16_decode` signature `proc utf16_decode(encoded: string) -> i32 {` example `encoding.utf16_decode(...)` stability `stable`
- `proc utf16_to_utf8` signature `proc utf16_to_utf8(text_value: string) -> string {` example `encoding.utf16_to_utf8(...)` stability `stable`
- `proc utf8_to_utf16` signature `proc utf8_to_utf16(text_value: string) -> string {` example `encoding.utf8_to_utf16(...)` stability `stable`
- `proc utf32_encode` signature `proc utf32_encode(code_point: i32) -> string {` example `encoding.utf32_encode(...)` stability `stable`
- `proc utf32_decode` signature `proc utf32_decode(encoded: string) -> i32 {` example `encoding.utf32_decode(...)` stability `stable`
- `const UNICODE_NFC` signature `const UNICODE_NFC: i32 = 1` example `encoding.UNICODE_NFC` stability `stable`
- `const UNICODE_NFD` signature `const UNICODE_NFD: i32 = 2` example `encoding.UNICODE_NFD` stability `stable`
- `const UNICODE_NFKC` signature `const UNICODE_NFKC: i32 = 3` example `encoding.UNICODE_NFKC` stability `stable`
- `const UNICODE_NFKD` signature `const UNICODE_NFKD: i32 = 4` example `encoding.UNICODE_NFKD` stability `stable`
- `proc unicode_normalize` signature `proc unicode_normalize(text_value: string, normalization_form: i32) -> string {` example `encoding.unicode_normalize(...)` stability `stable`
- `proc unicode_is_normalized` signature `proc unicode_is_normalized(text_value: string, normalization_form: i32) -> int {` example `encoding.unicode_is_normalized(...)` stability `stable`
- `proc char_code_point` signature `proc char_code_point(ch: i32) -> i32 {` example `encoding.char_code_point(...)` stability `stable`
- `proc code_point_to_char` signature `proc code_point_to_char(code: i32) -> i32 {` example `encoding.code_point_to_char(...)` stability `stable`
- `proc is_valid_code_point` signature `proc is_valid_code_point(code: i32) -> int {` example `encoding.is_valid_code_point(...)` stability `stable`
- `proc upcase_unicode` signature `proc upcase_unicode(text_value: string) -> string {` example `encoding.upcase_unicode(...)` stability `stable`
- `proc downcase_unicode` signature `proc downcase_unicode(text_value: string) -> string {` example `encoding.downcase_unicode(...)` stability `stable`
- `proc title_case_unicode` signature `proc title_case_unicode(text_value: string) -> string {` example `encoding.title_case_unicode(...)` stability `stable`
- `proc has_bom` signature `proc has_bom(data: string) -> int {` example `encoding.has_bom(...)` stability `stable`
- `proc add_bom_utf8` signature `proc add_bom_utf8(text_value: string) -> string {` example `encoding.add_bom_utf8(...)` stability `stable`
- `proc add_bom_utf16` signature `proc add_bom_utf16(text_value: string) -> string {` example `encoding.add_bom_utf16(...)` stability `stable`
- `proc remove_bom` signature `proc remove_bom(text_value: string) -> string {` example `encoding.remove_bom(...)` stability `stable`
- `proc detect_encoding` signature `proc detect_encoding(data: string) -> string {` example `encoding.detect_encoding(...)` stability `stable`
- `proc detect_encoding_confidence` signature `proc detect_encoding_confidence(data: string) -> f64 {` example `encoding.detect_encoding_confidence(...)` stability `stable`
- `proc convert_encoding` signature `proc convert_encoding(data: string, from_enc: string, to_enc: string) -> string {` example `encoding.convert_encoding(...)` stability `stable`
- `proc punycode_encode` signature `proc punycode_encode(text_value: string) -> string {` example `encoding.punycode_encode(...)` stability `stable`
- `proc punycode_decode` signature `proc punycode_decode(text_value: string) -> string {` example `encoding.punycode_decode(...)` stability `stable`
- `proc quote_printable_encode` signature `proc quote_printable_encode(data: string) -> string {` example `encoding.quote_printable_encode(...)` stability `stable`
- `proc quote_printable_decode` signature `proc quote_printable_decode(data: string) -> string {` example `encoding.quote_printable_decode(...)` stability `stable`
- `proc uuencode` signature `proc uuencode(data: string) -> string {` example `encoding.uuencode(...)` stability `stable`
- `proc uudecode` signature `proc uudecode(data: string) -> string {` example `encoding.uudecode(...)` stability `stable`

## `src/vitte/stdlib/json.vitl`

Stability: `stable`

- `pick JSONValue` signature `pick JSONValue {` example `json.JSONValue` stability `stable`
- `form JSONParser` signature `form JSONParser {` example `json.JSONParser` stability `stable`
- `form JSONBuilder` signature `form JSONBuilder {` example `json.JSONBuilder` stability `stable`
- `form JsonLibraryManifest` signature `form JsonLibraryManifest {` example `json.JsonLibraryManifest` stability `stable`
- `form JsonLibraryHealth` signature `form JsonLibraryHealth {` example `json.JsonLibraryHealth` stability `stable`
- `form JsonLibrarySummary` signature `form JsonLibrarySummary {` example `json.JsonLibrarySummary` stability `stable`
- `form JsonLibraryReport` signature `form JsonLibraryReport {` example `json.JsonLibraryReport` stability `stable`
- `form JsonParseReport` signature `form JsonParseReport {` example `json.JsonParseReport` stability `stable`
- `form JsonStringifyReport` signature `form JsonStringifyReport {` example `json.JsonStringifyReport` stability `stable`
- `form JsonBuilderReport` signature `form JsonBuilderReport {` example `json.JsonBuilderReport` stability `stable`
- `form JsonSchemaReport` signature `form JsonSchemaReport {` example `json.JsonSchemaReport` stability `stable`
- `proc json_version` signature `proc json_version() -> string {` example `json.json_version(...)` stability `stable`
- `proc json_name` signature `proc json_name() -> string {` example `json.json_name(...)` stability `stable`
- `proc json_module_count` signature `proc json_module_count() -> i32 {` example `json.json_module_count(...)` stability `stable`
- `proc json_modules` signature `proc json_modules() -> [string] {` example `json.json_modules(...)` stability `stable`
- `proc json_manifest` signature `proc json_manifest() -> JsonLibraryManifest {` example `json.json_manifest(...)` stability `stable`
- `proc json_ready` signature `proc json_ready() -> bool {` example `json.json_ready(...)` stability `stable`
- `proc json_health` signature `proc json_health() -> JsonLibraryHealth {` example `json.json_health(...)` stability `stable`
- `proc json_summary` signature `proc json_summary() -> JsonLibrarySummary {` example `json.json_summary(...)` stability `stable`
- `proc json_selftest` signature `proc json_selftest() -> bool {` example `json.json_selftest(...)` stability `stable`
- `proc json_report` signature `proc json_report() -> JsonLibraryReport {` example `json.json_report(...)` stability `stable`
- `proc json_parse` signature `proc json_parse(text_value: string) -> JSONValue {` example `json.json_parse(...)` stability `stable`
- `proc json_stringify` signature `proc json_stringify(value: JSONValue) -> string {` example `json.json_stringify(...)` stability `stable`
- `proc json_stringify_pretty` signature `proc json_stringify_pretty(value: JSONValue, indent: i32) -> string {` example `json.json_stringify_pretty(...)` stability `stable`
- `proc json_parse_object` signature `proc json_parse_object(parser: JSONParser) -> [string] {` example `json.json_parse_object(...)` stability `stable`
- `proc json_parse_array` signature `proc json_parse_array(parser: JSONParser) -> [JSONValue] {` example `json.json_parse_array(...)` stability `stable`
- `proc json_parse_string` signature `proc json_parse_string(parser: JSONParser) -> string {` example `json.json_parse_string(...)` stability `stable`
- `proc json_parse_number` signature `proc json_parse_number(parser: JSONParser) -> f64 {` example `json.json_parse_number(...)` stability `stable`
- `proc json_parse_bool` signature `proc json_parse_bool(parser: JSONParser) -> int {` example `json.json_parse_bool(...)` stability `stable`
- `proc json_parse_null` signature `proc json_parse_null(parser: JSONParser) -> int {` example `json.json_parse_null(...)` stability `stable`
- `proc json_builder_new` signature `proc json_builder_new() -> JSONBuilder {` example `json.json_builder_new(...)` stability `stable`
- `proc json_builder_append_null` signature `proc json_builder_append_null(b: JSONBuilder) -> int {` example `json.json_builder_append_null(...)` stability `stable`
- `proc json_builder_append_bool` signature `proc json_builder_append_bool(b: JSONBuilder, value: int) -> int {` example `json.json_builder_append_bool(...)` stability `stable`
- `proc json_builder_append_number` signature `proc json_builder_append_number(b: JSONBuilder, value: f64) -> int {` example `json.json_builder_append_number(...)` stability `stable`
- `proc json_builder_append_string` signature `proc json_builder_append_string(b: JSONBuilder, value: string) -> int {` example `json.json_builder_append_string(...)` stability `stable`
- `proc json_builder_start_object` signature `proc json_builder_start_object(b: JSONBuilder) -> int {` example `json.json_builder_start_object(...)` stability `stable`
- `proc json_builder_end_object` signature `proc json_builder_end_object(b: JSONBuilder) -> int {` example `json.json_builder_end_object(...)` stability `stable`
- `proc json_builder_start_array` signature `proc json_builder_start_array(b: JSONBuilder) -> int {` example `json.json_builder_start_array(...)` stability `stable`
- `proc json_builder_end_array` signature `proc json_builder_end_array(b: JSONBuilder) -> int {` example `json.json_builder_end_array(...)` stability `stable`
- `proc json_builder_append_comma` signature `proc json_builder_append_comma(b: JSONBuilder) -> int {` example `json.json_builder_append_comma(...)` stability `stable`
- `proc json_builder_append_colon` signature `proc json_builder_append_colon(b: JSONBuilder) -> int {` example `json.json_builder_append_colon(...)` stability `stable`
- `proc json_builder_to_string` signature `proc json_builder_to_string(b: JSONBuilder) -> string {` example `json.json_builder_to_string(...)` stability `stable`
- `proc json_builder_clear` signature `proc json_builder_clear(b: JSONBuilder) {` example `json.json_builder_clear(...)` stability `stable`
- `proc json_is_valid` signature `proc json_is_valid(text_value: string) -> int {` example `json.json_is_valid(...)` stability `stable`
- `proc json_format` signature `proc json_format(text_value: string) -> string {` example `json.json_format(...)` stability `stable`
- `proc json_minify` signature `proc json_minify(text_value: string) -> string {` example `json.json_minify(...)` stability `stable`
- `proc json_value_type` signature `proc json_value_type(value: JSONValue) -> string {` example `json.json_value_type(...)` stability `stable`
- `proc json_parse_report` signature `proc json_parse_report(text_value: string) -> JsonParseReport {` example `json.json_parse_report(...)` stability `stable`
- `proc json_stringify_report` signature `proc json_stringify_report(value: JSONValue) -> JsonStringifyReport {` example `json.json_stringify_report(...)` stability `stable`
- `proc json_builder_report` signature `proc json_builder_report() -> JsonBuilderReport {` example `json.json_builder_report(...)` stability `stable`
- `proc json_schema_report` signature `proc json_schema_report() -> JsonSchemaReport {` example `json.json_schema_report(...)` stability `stable`
- `proc json_max_report` signature `proc json_max_report() -> JsonLibrarySummary {` example `json.json_max_report(...)` stability `stable`

## `src/vitte/stdlib/crypto.vitl`

Stability: `stable`

- `form CryptoLibraryManifest` signature `form CryptoLibraryManifest {` example `crypto.CryptoLibraryManifest` stability `stable`
- `form CryptoLibraryHealth` signature `form CryptoLibraryHealth {` example `crypto.CryptoLibraryHealth` stability `stable`
- `form CryptoLibrarySummary` signature `form CryptoLibrarySummary {` example `crypto.CryptoLibrarySummary` stability `stable`
- `form CryptoLibraryReport` signature `form CryptoLibraryReport {` example `crypto.CryptoLibraryReport` stability `stable`
- `const SHA1_DIGEST_SIZE` signature `const SHA1_DIGEST_SIZE: i32 = 20` example `crypto.SHA1_DIGEST_SIZE` stability `stable`
- `const SHA256_DIGEST_SIZE` signature `const SHA256_DIGEST_SIZE: i32 = 32` example `crypto.SHA256_DIGEST_SIZE` stability `stable`
- `const SHA512_DIGEST_SIZE` signature `const SHA512_DIGEST_SIZE: i32 = 64` example `crypto.SHA512_DIGEST_SIZE` stability `stable`
- `const AES_ECB` signature `const AES_ECB: i32 = 1` example `crypto.AES_ECB` stability `stable`
- `const AES_CBC` signature `const AES_CBC: i32 = 2` example `crypto.AES_CBC` stability `stable`
- `const AES_CTR` signature `const AES_CTR: i32 = 3` example `crypto.AES_CTR` stability `stable`
- `const AES_GCM` signature `const AES_GCM: i32 = 4` example `crypto.AES_GCM` stability `stable`
- `const AES_128` signature `const AES_128: i32 = 16` example `crypto.AES_128` stability `stable`
- `const AES_192` signature `const AES_192: i32 = 24` example `crypto.AES_192` stability `stable`
- `const AES_256` signature `const AES_256: i32 = 32` example `crypto.AES_256` stability `stable`
- `form Hash` signature `form Hash {` example `crypto.Hash` stability `stable`
- `form HMAC` signature `form HMAC {` example `crypto.HMAC` stability `stable`
- `proc crypto_version` signature `proc crypto_version() -> string {` example `crypto.crypto_version(...)` stability `stable`
- `proc crypto_name` signature `proc crypto_name() -> string {` example `crypto.crypto_name(...)` stability `stable`
- `proc crypto_modules` signature `proc crypto_modules() -> [string] {` example `crypto.crypto_modules(...)` stability `stable`
- `proc crypto_module_count` signature `proc crypto_module_count() -> i32 {` example `crypto.crypto_module_count(...)` stability `stable`
- `proc crypto_manifest` signature `proc crypto_manifest() -> CryptoLibraryManifest {` example `crypto.crypto_manifest(...)` stability `stable`
- `proc crypto_ready` signature `proc crypto_ready() -> bool {` example `crypto.crypto_ready(...)` stability `stable`
- `proc crypto_health` signature `proc crypto_health() -> CryptoLibraryHealth {` example `crypto.crypto_health(...)` stability `stable`
- `proc crypto_summary` signature `proc crypto_summary() -> CryptoLibrarySummary {` example `crypto.crypto_summary(...)` stability `stable`
- `proc crypto_report` signature `proc crypto_report() -> CryptoLibraryReport {` example `crypto.crypto_report(...)` stability `stable`
- `proc md5` signature `proc md5(data: string) -> string {` example `crypto.md5(...)` stability `stable`
- `proc md5_hex` signature `proc md5_hex(data: string) -> string {` example `crypto.md5_hex(...)` stability `stable`
- `proc sha1` signature `proc sha1(data: string) -> string {` example `crypto.sha1(...)` stability `stable`
- `proc sha1_hex` signature `proc sha1_hex(data: string) -> string {` example `crypto.sha1_hex(...)` stability `stable`
- `proc sha256` signature `proc sha256(data: string) -> string {` example `crypto.sha256(...)` stability `stable`
- `proc sha256_hex` signature `proc sha256_hex(data: string) -> string {` example `crypto.sha256_hex(...)` stability `stable`
- `proc sha512` signature `proc sha512(data: string) -> string {` example `crypto.sha512(...)` stability `stable`
- `proc sha512_hex` signature `proc sha512_hex(data: string) -> string {` example `crypto.sha512_hex(...)` stability `stable`
- `proc sha3_256` signature `proc sha3_256(data: string) -> string {` example `crypto.sha3_256(...)` stability `stable`
- `proc sha3_256_hex` signature `proc sha3_256_hex(data: string) -> string {` example `crypto.sha3_256_hex(...)` stability `stable`
- `proc sha3_512` signature `proc sha3_512(data: string) -> string {` example `crypto.sha3_512(...)` stability `stable`
- `proc sha3_512_hex` signature `proc sha3_512_hex(data: string) -> string {` example `crypto.sha3_512_hex(...)` stability `stable`
- `proc blake2b` signature `proc blake2b(data: string, size: i32) -> string {` example `crypto.blake2b(...)` stability `stable`
- `proc blake2b_hex` signature `proc blake2b_hex(data: string, size: i32) -> string {` example `crypto.blake2b_hex(...)` stability `stable`
- `proc hash_new` signature `proc hash_new(algorithm: i32) -> Hash {` example `crypto.hash_new(...)` stability `stable`
- `proc hash_update` signature `proc hash_update(h: Hash, data: string) -> int {` example `crypto.hash_update(...)` stability `stable`
- `proc hash_final` signature `proc hash_final(h: Hash) -> string {` example `crypto.hash_final(...)` stability `stable`
- `proc hash_final_hex` signature `proc hash_final_hex(h: Hash) -> string {` example `crypto.hash_final_hex(...)` stability `stable`
- `proc hmac_new` signature `proc hmac_new(algorithm: i32, key: string) -> HMAC {` example `crypto.hmac_new(...)` stability `stable`
- `proc hmac_update` signature `proc hmac_update(h: HMAC, data: string) -> int {` example `crypto.hmac_update(...)` stability `stable`
- `proc hmac_final` signature `proc hmac_final(h: HMAC) -> string {` example `crypto.hmac_final(...)` stability `stable`
- `proc hmac_final_hex` signature `proc hmac_final_hex(h: HMAC) -> string {` example `crypto.hmac_final_hex(...)` stability `stable`
- `proc hash_compare` signature `proc hash_compare(hash1: string, hash2: string) -> int {` example `crypto.hash_compare(...)` stability `stable`
- `proc hash_file` signature `proc hash_file(filepath: string, algorithm: i32) -> string {` example `crypto.hash_file(...)` stability `stable`
- `proc hash_file_hex` signature `proc hash_file_hex(filepath: string, algorithm: i32) -> string {` example `crypto.hash_file_hex(...)` stability `stable`
- `proc random_bytes` signature `proc random_bytes(size: i32) -> string {` example `crypto.random_bytes(...)` stability `stable`
- `proc random_bytes_hex` signature `proc random_bytes_hex(size: i32) -> string {` example `crypto.random_bytes_hex(...)` stability `stable`
- `proc pbkdf2` signature `proc pbkdf2(password: string, salt: string, iterations: i32, length: i32) -> string {` example `crypto.pbkdf2(...)` stability `stable`
- `proc bcrypt_hash` signature `proc bcrypt_hash(password: string, rounds: i32) -> string {` example `crypto.bcrypt_hash(...)` stability `stable`
- `proc bcrypt_verify` signature `proc bcrypt_verify(password: string, hash: string) -> int {` example `crypto.bcrypt_verify(...)` stability `stable`
- `proc crypt_encode_base64` signature `proc crypt_encode_base64(data: string) -> string {` example `crypto.crypt_encode_base64(...)` stability `stable`
- `proc crypt_decode_base64` signature `proc crypt_decode_base64(data: string) -> string {` example `crypto.crypt_decode_base64(...)` stability `stable`
- `proc crypt_encode_hex` signature `proc crypt_encode_hex(data: string) -> string {` example `crypto.crypt_encode_hex(...)` stability `stable`
- `proc crypt_decode_hex` signature `proc crypt_decode_hex(data: string) -> string {` example `crypto.crypt_decode_hex(...)` stability `stable`
- `proc crypto_selftest` signature `proc crypto_selftest() -> bool {` example `crypto.crypto_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression.vitl`

Stability: `stable`

- `const Z_FAST_COMPRESSION` signature `const Z_FAST_COMPRESSION: i32 = 1` example `compression.Z_FAST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_COMPRESSION` signature `const Z_DEFAULT_COMPRESSION: i32 = 6` example `compression.Z_DEFAULT_COMPRESSION` stability `stable`
- `const Z_BEST_COMPRESSION` signature `const Z_BEST_COMPRESSION: i32 = 9` example `compression.Z_BEST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_STRATEGY` signature `const Z_DEFAULT_STRATEGY: i32 = 0` example `compression.Z_DEFAULT_STRATEGY` stability `stable`
- `const Z_FILTERED` signature `const Z_FILTERED: i32 = 1` example `compression.Z_FILTERED` stability `stable`
- `const Z_HUFFMAN_ONLY` signature `const Z_HUFFMAN_ONLY: i32 = 2` example `compression.Z_HUFFMAN_ONLY` stability `stable`
- `const Z_RLE` signature `const Z_RLE: i32 = 3` example `compression.Z_RLE` stability `stable`
- `const ALGO_DEFLATE` signature `const ALGO_DEFLATE: i32 = 1` example `compression.ALGO_DEFLATE` stability `stable`
- `const ALGO_GZIP` signature `const ALGO_GZIP: i32 = 2` example `compression.ALGO_GZIP` stability `stable`
- `const ALGO_ZLIB` signature `const ALGO_ZLIB: i32 = 3` example `compression.ALGO_ZLIB` stability `stable`
- `const ALGO_RLE` signature `const ALGO_RLE: i32 = 4` example `compression.ALGO_RLE` stability `stable`
- `const ALGO_HUFFMAN` signature `const ALGO_HUFFMAN: i32 = 5` example `compression.ALGO_HUFFMAN` stability `stable`
- `const ALGO_LZ77` signature `const ALGO_LZ77: i32 = 6` example `compression.ALGO_LZ77` stability `stable`
- `const ALGO_BROTLI` signature `const ALGO_BROTLI: i32 = 7` example `compression.ALGO_BROTLI` stability `stable`
- `const ERR_INVALID_ALGO` signature `const ERR_INVALID_ALGO: i32 = -1` example `compression.ERR_INVALID_ALGO` stability `stable`
- `const ERR_INVALID_DATA` signature `const ERR_INVALID_DATA: i32 = -2` example `compression.ERR_INVALID_DATA` stability `stable`
- `const ERR_EMPTY` signature `const ERR_EMPTY: i32 = -3` example `compression.ERR_EMPTY` stability `stable`
- `form CompressionStats` signature `form CompressionStats {` example `compression.CompressionStats` stability `stable`
- `form CompressionManifest` signature `form CompressionManifest {` example `compression.CompressionManifest` stability `stable`
- `form CompressionHealth` signature `form CompressionHealth {` example `compression.CompressionHealth` stability `stable`
- `form CompressionSummary` signature `form CompressionSummary {` example `compression.CompressionSummary` stability `stable`
- `form Compressor` signature `form Compressor {` example `compression.Compressor` stability `stable`
- `form Decompressor` signature `form Decompressor {` example `compression.Decompressor` stability `stable`
- `pick CompressionResult` signature `pick CompressionResult {` example `compression.CompressionResult` stability `stable`
- `proc compression_version` signature `proc compression_version() -> string {` example `compression.compression_version(...)` stability `stable`
- `proc compression_algorithms` signature `proc compression_algorithms() -> [string] {` example `compression.compression_algorithms(...)` stability `stable`
- `proc compression_manifest` signature `proc compression_manifest() -> CompressionManifest {` example `compression.compression_manifest(...)` stability `stable`
- `proc compression_ready` signature `proc compression_ready() -> bool {` example `compression.compression_ready(...)` stability `stable`
- `proc compression_health` signature `proc compression_health() -> CompressionHealth {` example `compression.compression_health(...)` stability `stable`
- `proc compression_summary` signature `proc compression_summary() -> CompressionSummary {` example `compression.compression_summary(...)` stability `stable`
- `proc _char_to_digit` signature `proc _char_to_digit(ch: char) -> i32 {` example `compression._char_to_digit(...)` stability `stable`
- `proc _is_digit` signature `proc _is_digit(ch: char) -> bool {` example `compression._is_digit(...)` stability `stable`
- `proc _escape_marker` signature `proc _escape_marker(text: string) -> string {` example `compression._escape_marker(...)` stability `stable`
- `proc _unescape_marker` signature `proc _unescape_marker(text: string) -> string {` example `compression._unescape_marker(...)` stability `stable`
- `proc _parse_int` signature `proc _parse_int(text: string, start: i32, end: i32) -> i32 {` example `compression._parse_int(...)` stability `stable`
- `proc _read_run` signature `proc _read_run(text: string, index: i32) -> [i32] {` example `compression._read_run(...)` stability `stable`
- `proc compress_rle` signature `proc compress_rle(data: string) -> string {` example `compression.compress_rle(...)` stability `stable`
- `proc decompress_rle` signature `proc decompress_rle(data: string) -> string {` example `compression.decompress_rle(...)` stability `stable`
- `proc _wrap_algo` signature `proc _wrap_algo(tag: string, data: string, level: i32) -> string {` example `compression._wrap_algo(...)` stability `stable`
- `proc _unwrap_algo` signature `proc _unwrap_algo(tag: string, data: string) -> string {` example `compression._unwrap_algo(...)` stability `stable`
- `proc _slice_text` signature `proc _slice_text(text: string, start: i32, end: i32) -> string {` example `compression._slice_text(...)` stability `stable`
- `proc compress_deflate` signature `proc compress_deflate(data: string, level: i32) -> string {` example `compression.compress_deflate(...)` stability `stable`
- `proc decompress_deflate` signature `proc decompress_deflate(data: string) -> string {` example `compression.decompress_deflate(...)` stability `stable`
- `proc compress_gzip` signature `proc compress_gzip(data: string, level: i32) -> string {` example `compression.compress_gzip(...)` stability `stable`
- `proc decompress_gzip` signature `proc decompress_gzip(data: string) -> string {` example `compression.decompress_gzip(...)` stability `stable`
- `proc compress_zlib` signature `proc compress_zlib(data: string, level: i32) -> string {` example `compression.compress_zlib(...)` stability `stable`
- `proc decompress_zlib` signature `proc decompress_zlib(data: string) -> string {` example `compression.decompress_zlib(...)` stability `stable`
- `proc compress_huffman` signature `proc compress_huffman(data: string) -> string {` example `compression.compress_huffman(...)` stability `stable`
- `proc decompress_huffman` signature `proc decompress_huffman(data: string) -> string {` example `compression.decompress_huffman(...)` stability `stable`
- `proc compress_lz77` signature `proc compress_lz77(data: string) -> string {` example `compression.compress_lz77(...)` stability `stable`
- `proc decompress_lz77` signature `proc decompress_lz77(data: string) -> string {` example `compression.decompress_lz77(...)` stability `stable`
- `proc compress_brotli` signature `proc compress_brotli(data: string, level: i32) -> string {` example `compression.compress_brotli(...)` stability `stable`
- `proc decompress_brotli` signature `proc decompress_brotli(data: string) -> string {` example `compression.decompress_brotli(...)` stability `stable`
- `proc compressor_new` signature `proc compressor_new(level: i32) -> Compressor {` example `compression.compressor_new(...)` stability `stable`
- `proc compressor_set_strategy` signature `proc compressor_set_strategy(c: Compressor, strategy: i32) -> int {` example `compression.compressor_set_strategy(...)` stability `stable`
- `proc compressor_set_algorithm` signature `proc compressor_set_algorithm(c: Compressor, algorithm: i32) -> int {` example `compression.compressor_set_algorithm(...)` stability `stable`
- `proc compressor_compress` signature `proc compressor_compress(c: Compressor, data: string) -> string {` example `compression.compressor_compress(...)` stability `stable`
- `proc compressor_flush` signature `proc compressor_flush(c: Compressor) -> string {` example `compression.compressor_flush(...)` stability `stable`
- `proc compressor_reset` signature `proc compressor_reset(c: Compressor) -> int {` example `compression.compressor_reset(...)` stability `stable`
- `proc decompressor_new` signature `proc decompressor_new() -> Decompressor {` example `compression.decompressor_new(...)` stability `stable`
- `proc decompressor_set_algorithm` signature `proc decompressor_set_algorithm(d: Decompressor, algorithm: i32) -> int {` example `compression.decompressor_set_algorithm(...)` stability `stable`
- `proc decompressor_decompress` signature `proc decompressor_decompress(d: Decompressor, data: string) -> string {` example `compression.decompressor_decompress(...)` stability `stable`
- `proc decompressor_flush` signature `proc decompressor_flush(d: Decompressor) -> string {` example `compression.decompressor_flush(...)` stability `stable`
- `proc decompressor_reset` signature `proc decompressor_reset(d: Decompressor) -> int {` example `compression.decompressor_reset(...)` stability `stable`
- `proc compress` signature `proc compress(data: string, algo: i32, level: i32) -> string {` example `compression.compress(...)` stability `stable`
- `proc decompress` signature `proc decompress(data: string, algo: i32) -> string {` example `compression.decompress(...)` stability `stable`
- `proc compress_ex` signature `proc compress_ex(data: string, algo: i32, level: i32, strategy: i32) -> string {` example `compression.compress_ex(...)` stability `stable`
- `proc get_compression_ratio` signature `proc get_compression_ratio(original_size: i64, compressed_size: i64) -> f64 {` example `compression.get_compression_ratio(...)` stability `stable`
- `proc estimate_compressed_size` signature `proc estimate_compressed_size(data: string) -> i64 {` example `compression.estimate_compressed_size(...)` stability `stable`
- `proc get_best_compression_level` signature `proc get_best_compression_level(data: string) -> i32 {` example `compression.get_best_compression_level(...)` stability `stable`
- `proc compression_selftest` signature `proc compression_selftest() -> bool {` example `compression.compression_selftest(...)` stability `stable`

## `src/vitte/stdlib/regex.vitl`

Stability: `stable`

- `form Regex` signature `form Regex {` example `regex.Regex` stability `stable`
- `form Match` signature `form Match {` example `regex.Match` stability `stable`
- `form MatchResult` signature `form MatchResult {` example `regex.MatchResult` stability `stable`
- `form RegexLibraryManifest` signature `form RegexLibraryManifest {` example `regex.RegexLibraryManifest` stability `stable`
- `form RegexLibraryHealth` signature `form RegexLibraryHealth {` example `regex.RegexLibraryHealth` stability `stable`
- `form RegexLibrarySummary` signature `form RegexLibrarySummary {` example `regex.RegexLibrarySummary` stability `stable`
- `form RegexParseReport` signature `form RegexParseReport {` example `regex.RegexParseReport` stability `stable`
- `form RegexMatchReport` signature `form RegexMatchReport {` example `regex.RegexMatchReport` stability `stable`
- `form RegexReplaceReport` signature `form RegexReplaceReport {` example `regex.RegexReplaceReport` stability `stable`
- `proc regex_version` signature `proc regex_version() -> string {` example `regex.regex_version(...)` stability `stable`
- `proc regex_name` signature `proc regex_name() -> string {` example `regex.regex_name(...)` stability `stable`
- `proc regex_module_count` signature `proc regex_module_count() -> i32 {` example `regex.regex_module_count(...)` stability `stable`
- `proc regex_modules` signature `proc regex_modules() -> [string] {` example `regex.regex_modules(...)` stability `stable`
- `proc regex_manifest` signature `proc regex_manifest() -> RegexLibraryManifest {` example `regex.regex_manifest(...)` stability `stable`
- `proc regex_ready` signature `proc regex_ready() -> bool {` example `regex.regex_ready(...)` stability `stable`
- `proc regex_health` signature `proc regex_health() -> RegexLibraryHealth {` example `regex.regex_health(...)` stability `stable`
- `proc regex_summary` signature `proc regex_summary() -> RegexLibrarySummary {` example `regex.regex_summary(...)` stability `stable`
- `proc regex_selftest` signature `proc regex_selftest() -> bool {` example `regex.regex_selftest(...)` stability `stable`
- `proc regex_compile` signature `proc regex_compile(pattern: string) -> Regex {` example `regex.regex_compile(...)` stability `stable`
- `proc regex_compile_flags` signature `proc regex_compile_flags(pattern: string, flag_bits: i32) -> Regex {` example `regex.regex_compile_flags(...)` stability `stable`
- `proc regex_is_valid` signature `proc regex_is_valid(pattern: string) -> int {` example `regex.regex_is_valid(...)` stability `stable`
- `proc regex_match` signature `proc regex_match(re: Regex, text_value: string) -> int {` example `regex.regex_match(...)` stability `stable`
- `proc regex_match_at` signature `proc regex_match_at(re: Regex, text_value: string, pos: i32) -> int {` example `regex.regex_match_at(...)` stability `stable`
- `proc regex_find` signature `proc regex_find(re: Regex, text_value: string) -> Match {` example `regex.regex_find(...)` stability `stable`
- `proc regex_find_all` signature `proc regex_find_all(re: Regex, text_value: string) -> MatchResult {` example `regex.regex_find_all(...)` stability `stable`
- `proc regex_get_group` signature `proc regex_get_group(m: Match, group: i32) -> string {` example `regex.regex_get_group(...)` stability `stable`
- `proc regex_group_count` signature `proc regex_group_count(m: Match) -> i32 {` example `regex.regex_group_count(...)` stability `stable`
- `proc regex_replace` signature `proc regex_replace(re: Regex, text_value: string, replacement: string) -> string {` example `regex.regex_replace(...)` stability `stable`
- `proc regex_replace_first` signature `proc regex_replace_first(re: Regex, text_value: string, replacement: string) -> string {` example `regex.regex_replace_first(...)` stability `stable`
- `proc regex_replace_all` signature `proc regex_replace_all(re: Regex, text_value: string, replacement: string) -> string {` example `regex.regex_replace_all(...)` stability `stable`
- `proc regex_split` signature `proc regex_split(re: Regex, text_value: string) -> [string] {` example `regex.regex_split(...)` stability `stable`
- `proc regex_split_limit` signature `proc regex_split_limit(re: Regex, text_value: string, limit: i32) -> [string] {` example `regex.regex_split_limit(...)` stability `stable`
- `proc regex_test` signature `proc regex_test(re: Regex, text_value: string) -> int {` example `regex.regex_test(...)` stability `stable`
- `proc regex_test_at` signature `proc regex_test_at(re: Regex, text_value: string, pos: i32) -> int {` example `regex.regex_test_at(...)` stability `stable`
- `proc regex_escape` signature `proc regex_escape(text_value: string) -> string {` example `regex.regex_escape(...)` stability `stable`
- `proc regex_create_pattern` signature `proc regex_create_pattern(parts: [string], separator: string) -> string {` example `regex.regex_create_pattern(...)` stability `stable`
- `proc str_matches` signature `proc str_matches(text_value: string, pattern: string) -> int {` example `regex.str_matches(...)` stability `stable`
- `proc str_matches_at` signature `proc str_matches_at(text_value: string, pattern: string, pos: i32) -> int {` example `regex.str_matches_at(...)` stability `stable`
- `proc str_find_match` signature `proc str_find_match(text_value: string, pattern: string) -> Match {` example `regex.str_find_match(...)` stability `stable`
- `proc str_find_all_matches` signature `proc str_find_all_matches(text_value: string, pattern: string) -> MatchResult {` example `regex.str_find_all_matches(...)` stability `stable`
- `proc str_replace_regex` signature `proc str_replace_regex(text_value: string, pattern: string, replacement: string) -> string {` example `regex.str_replace_regex(...)` stability `stable`
- `proc str_split_regex` signature `proc str_split_regex(text_value: string, pattern: string) -> [string] {` example `regex.str_split_regex(...)` stability `stable`
- `const REGEX_EMAIL` signature `const REGEX_EMAIL: string = ""` example `regex.REGEX_EMAIL` stability `stable`
- `const REGEX_URL` signature `const REGEX_URL: string = ""` example `regex.REGEX_URL` stability `stable`
- `const REGEX_IPV4` signature `const REGEX_IPV4: string = ""` example `regex.REGEX_IPV4` stability `stable`
- `const REGEX_IPV6` signature `const REGEX_IPV6: string = ""` example `regex.REGEX_IPV6` stability `stable`
- `const REGEX_PHONE` signature `const REGEX_PHONE: string = ""` example `regex.REGEX_PHONE` stability `stable`
- `const REGEX_DATE` signature `const REGEX_DATE: string = ""` example `regex.REGEX_DATE` stability `stable`
- `const REGEX_TIME` signature `const REGEX_TIME: string = ""` example `regex.REGEX_TIME` stability `stable`
- `const REGEX_UUID` signature `const REGEX_UUID: string = ""` example `regex.REGEX_UUID` stability `stable`
- `const REGEX_CREDIT_CARD` signature `const REGEX_CREDIT_CARD: string = ""` example `regex.REGEX_CREDIT_CARD` stability `stable`
- `proc is_email` signature `proc is_email(text_value: string) -> int {` example `regex.is_email(...)` stability `stable`
- `proc is_url` signature `proc is_url(text_value: string) -> int {` example `regex.is_url(...)` stability `stable`
- `proc is_ipv4` signature `proc is_ipv4(text_value: string) -> int {` example `regex.is_ipv4(...)` stability `stable`
- `proc is_ipv6` signature `proc is_ipv6(text_value: string) -> int {` example `regex.is_ipv6(...)` stability `stable`
- `proc is_phone` signature `proc is_phone(text_value: string) -> int {` example `regex.is_phone(...)` stability `stable`
- `proc is_date` signature `proc is_date(text_value: string) -> int {` example `regex.is_date(...)` stability `stable`
- `proc is_time` signature `proc is_time(text_value: string) -> int {` example `regex.is_time(...)` stability `stable`
- `proc is_uuid` signature `proc is_uuid(text_value: string) -> int {` example `regex.is_uuid(...)` stability `stable`
- `proc regex_parse_report` signature `proc regex_parse_report(pattern: string) -> RegexParseReport {` example `regex.regex_parse_report(...)` stability `stable`
- `proc regex_match_report` signature `proc regex_match_report(pattern: string, text: string) -> RegexMatchReport {` example `regex.regex_match_report(...)` stability `stable`
- `proc regex_replace_report` signature `proc regex_replace_report(pattern: string, text: string) -> RegexReplaceReport {` example `regex.regex_replace_report(...)` stability `stable`

## `src/vitte/stdlib/io.vitl`

Stability: `stable`

- `const IO_VERSION` signature `const IO_VERSION: string = "1.0.0"` example `io.IO_VERSION` stability `stable`
- `const EOF` signature `const EOF: int = -1` example `io.EOF` stability `stable`
- `const NULL_FD` signature `const NULL_FD: int = -1` example `io.NULL_FD` stability `stable`
- `const SEEK_SET` signature `const SEEK_SET: int = 0` example `io.SEEK_SET` stability `stable`
- `const SEEK_CUR` signature `const SEEK_CUR: int = 1` example `io.SEEK_CUR` stability `stable`
- `const SEEK_END` signature `const SEEK_END: int = 2` example `io.SEEK_END` stability `stable`
- `const IO_READ` signature `const IO_READ: int = 1` example `io.IO_READ` stability `stable`
- `const IO_WRITE` signature `const IO_WRITE: int = 2` example `io.IO_WRITE` stability `stable`
- `const IO_APPEND` signature `const IO_APPEND: int = 4` example `io.IO_APPEND` stability `stable`
- `const IO_CREATE` signature `const IO_CREATE: int = 8` example `io.IO_CREATE` stability `stable`
- `const IO_TRUNC` signature `const IO_TRUNC: int = 16` example `io.IO_TRUNC` stability `stable`
- `const IO_BINARY` signature `const IO_BINARY: int = 32` example `io.IO_BINARY` stability `stable`
- `const IO_TEXT` signature `const IO_TEXT: int = 64` example `io.IO_TEXT` stability `stable`
- `const READ` signature `const READ: string = "r"` example `io.READ` stability `stable`
- `const WRITE` signature `const WRITE: string = "w"` example `io.WRITE` stability `stable`
- `const APPEND` signature `const APPEND: string = "a"` example `io.APPEND` stability `stable`
- `const READ_WRITE` signature `const READ_WRITE: string = "r+"` example `io.READ_WRITE` stability `stable`
- `const WRITE_READ` signature `const WRITE_READ: string = "w+"` example `io.WRITE_READ` stability `stable`
- `const APPEND_READ` signature `const APPEND_READ: string = "a+"` example `io.APPEND_READ` stability `stable`
- `const READ_BINARY` signature `const READ_BINARY: string = "rb"` example `io.READ_BINARY` stability `stable`
- `const WRITE_BINARY` signature `const WRITE_BINARY: string = "wb"` example `io.WRITE_BINARY` stability `stable`
- `const APPEND_BINARY` signature `const APPEND_BINARY: string = "ab"` example `io.APPEND_BINARY` stability `stable`
- `const _IOFBF` signature `const _IOFBF: i32 = 0` example `io._IOFBF` stability `stable`
- `const _IOLBF` signature `const _IOLBF: i32 = 1` example `io._IOLBF` stability `stable`
- `const _IONBF` signature `const _IONBF: i32 = 2` example `io._IONBF` stability `stable`
- `const PATH_SEP` signature `const PATH_SEP: string = "/"` example `io.PATH_SEP` stability `stable`
- `const EXT_SEP` signature `const EXT_SEP: string = "."` example `io.EXT_SEP` stability `stable`
- `const MAX_PATH` signature `const MAX_PATH: int = 4096` example `io.MAX_PATH` stability `stable`
- `pick IoStatus` signature `pick IoStatus {` example `io.IoStatus` stability `stable`
- `form IoResult` signature `form IoResult {` example `io.IoResult` stability `stable`
- `form File` signature `form File {` example `io.File` stability `stable`
- `form FileStat` signature `form FileStat {` example `io.FileStat` stability `stable`
- `form DirEntry` signature `form DirEntry {` example `io.DirEntry` stability `stable`
- `form Buffer` signature `form Buffer {` example `io.Buffer` stability `stable`
- `form TextReader` signature `form TextReader {` example `io.TextReader` stability `stable`
- `form TextWriter` signature `form TextWriter {` example `io.TextWriter` stability `stable`
- `form PathInfo` signature `form PathInfo {` example `io.PathInfo` stability `stable`
- `form IOLibraryManifest` signature `form IOLibraryManifest {` example `io.IOLibraryManifest` stability `stable`
- `form IOLibraryHealth` signature `form IOLibraryHealth {` example `io.IOLibraryHealth` stability `stable`
- `form IOLibrarySummary` signature `form IOLibrarySummary {` example `io.IOLibrarySummary` stability `stable`
- `proc io_ok` signature `proc io_ok(message: string) -> IoResult {` example `io.io_ok(...)` stability `stable`
- `proc io_name` signature `proc io_name() -> string {` example `io.io_name(...)` stability `stable`
- `proc io_version` signature `proc io_version() -> string {` example `io.io_version(...)` stability `stable`
- `proc io_modules` signature `proc io_modules() -> [string] {` example `io.io_modules(...)` stability `stable`
- `proc io_module_count` signature `proc io_module_count() -> int {` example `io.io_module_count(...)` stability `stable`
- `proc io_manifest` signature `proc io_manifest() -> IOLibraryManifest {` example `io.io_manifest(...)` stability `stable`
- `proc io_health` signature `proc io_health() -> IOLibraryHealth {` example `io.io_health(...)` stability `stable`
- `proc io_summary` signature `proc io_summary() -> IOLibrarySummary {` example `io.io_summary(...)` stability `stable`
- `proc io_failed` signature `proc io_failed(code: int, message: string) -> IoResult {` example `io.io_failed(...)` stability `stable`
- `proc null_file` signature `proc null_file() -> File {` example `io.null_file(...)` stability `stable`
- `proc stdin_file` signature `proc stdin_file() -> File {` example `io.stdin_file(...)` stability `stable`
- `proc stdout_file` signature `proc stdout_file() -> File {` example `io.stdout_file(...)` stability `stable`
- `proc stderr_file` signature `proc stderr_file() -> File {` example `io.stderr_file(...)` stability `stable`
- `proc file_is_open` signature `proc file_is_open(f: File) -> bool {` example `io.file_is_open(...)` stability `stable`
- `proc file_can_read` signature `proc file_can_read(f: File) -> bool {` example `io.file_can_read(...)` stability `stable`
- `proc file_can_write` signature `proc file_can_write(f: File) -> bool {` example `io.file_can_write(...)` stability `stable`
- `proc mode_readable` signature `proc mode_readable(mode: string) -> bool {` example `io.mode_readable(...)` stability `stable`
- `proc mode_writable` signature `proc mode_writable(mode: string) -> bool {` example `io.mode_writable(...)` stability `stable`
- `proc mode_append` signature `proc mode_append(mode: string) -> bool {` example `io.mode_append(...)` stability `stable`
- `proc mode_binary` signature `proc mode_binary(mode: string) -> bool {` example `io.mode_binary(...)` stability `stable`
- `proc fopen` signature `proc fopen(path: string, mode: string) -> File {` example `io.fopen(...)` stability `stable`
- `proc freopen` signature `proc freopen(path: string, mode: string, stream: File) -> File {` example `io.freopen(...)` stability `stable`
- `proc fclose` signature `proc fclose(f: File) -> int {` example `io.fclose(...)` stability `stable`
- `proc fflush` signature `proc fflush(f: File) -> int {` example `io.fflush(...)` stability `stable`
- `proc ferror` signature `proc ferror(f: File) -> int {` example `io.ferror(...)` stability `stable`
- `proc feof` signature `proc feof(f: File) -> bool {` example `io.feof(...)` stability `stable`
- `proc clearerr` signature `proc clearerr(f: File) -> File {` example `io.clearerr(...)` stability `stable`
- `proc rewind` signature `proc rewind(f: File) -> File {` example `io.rewind(...)` stability `stable`
- `proc fseek` signature `proc fseek(f: File, offset: i64, whence: int) -> File {` example `io.fseek(...)` stability `stable`
- `proc ftell` signature `proc ftell(f: File) -> i64 {` example `io.ftell(...)` stability `stable`
- `proc fgetpos` signature `proc fgetpos(f: File) -> i64 {` example `io.fgetpos(...)` stability `stable`
- `proc fsetpos` signature `proc fsetpos(f: File, pos: i64) -> File {` example `io.fsetpos(...)` stability `stable`
- `proc fread` signature `proc fread(buffer: bytes, size: usize, count: usize, f: File) -> usize {` example `io.fread(...)` stability `stable`
- `proc fwrite` signature `proc fwrite(buffer: bytes, size: usize, count: usize, f: File) -> usize {` example `io.fwrite(...)` stability `stable`
- `proc fgetc` signature `proc fgetc(f: File) -> int {` example `io.fgetc(...)` stability `stable`
- `proc getc` signature `proc getc(f: File) -> int {` example `io.getc(...)` stability `stable`
- `proc getchar` signature `proc getchar() -> int {` example `io.getchar(...)` stability `stable`
- `proc fputc` signature `proc fputc(c: int, f: File) -> int {` example `io.fputc(...)` stability `stable`
- `proc putc` signature `proc putc(c: int, f: File) -> int {` example `io.putc(...)` stability `stable`
- `proc putchar` signature `proc putchar(c: int) -> int {` example `io.putchar(...)` stability `stable`
- `proc ungetc` signature `proc ungetc(c: int, f: File) -> int {` example `io.ungetc(...)` stability `stable`
- `proc fgets` signature `proc fgets(size: int, f: File) -> string {` example `io.fgets(...)` stability `stable`
- `proc gets` signature `proc gets() -> string {` example `io.gets(...)` stability `stable`
- `proc fputs` signature `proc fputs(s: string, f: File) -> int {` example `io.fputs(...)` stability `stable`
- `proc puts` signature `proc puts(s: string) -> int {` example `io.puts(...)` stability `stable`
- `proc print` signature `proc print(s: string) -> void {` example `io.print(...)` stability `stable`
- `proc println` signature `proc println(s: string) -> void {` example `io.println(...)` stability `stable`
- `proc eprint` signature `proc eprint(s: string) -> void {` example `io.eprint(...)` stability `stable`
- `proc eprintln` signature `proc eprintln(s: string) -> void {` example `io.eprintln(...)` stability `stable`
- `proc emit_string` signature `proc emit_string(s: string) -> void {` example `io.emit_string(...)` stability `stable`
- `proc print_int` signature `proc print_int(value: i64) -> void {` example `io.print_int(...)` stability `stable`
- `proc println_int` signature `proc println_int(value: i64) -> void {` example `io.println_int(...)` stability `stable`
- `proc print_float` signature `proc print_float(value: f64) -> void {` example `io.print_float(...)` stability `stable`
- `proc println_float` signature `proc println_float(value: f64) -> void {` example `io.println_float(...)` stability `stable`
- `proc print_bool` signature `proc print_bool(value: bool) -> void {` example `io.print_bool(...)` stability `stable`
- `proc println_bool` signature `proc println_bool(value: bool) -> void {` example `io.println_bool(...)` stability `stable`
- `proc printf` signature `proc printf(format: string, args: [string]) -> int {` example `io.printf(...)` stability `stable`
- `proc fprintf` signature `proc fprintf(f: File, format: string, args: [string]) -> int {` example `io.fprintf(...)` stability `stable`
- `proc sprintf` signature `proc sprintf(format: string, args: [string]) -> string {` example `io.sprintf(...)` stability `stable`
- `proc snprintf` signature `proc snprintf(size: usize, format: string, args: [string]) -> string {` example `io.snprintf(...)` stability `stable`
- `proc scanf` signature `proc scanf(format: string) -> int {` example `io.scanf(...)` stability `stable`
- `proc fscanf` signature `proc fscanf(f: File, format: string) -> int {` example `io.fscanf(...)` stability `stable`
- `proc sscanf` signature `proc sscanf(input: string, format: string) -> int {` example `io.sscanf(...)` stability `stable`
- `proc read_line` signature `proc read_line() -> string {` example `io.read_line(...)` stability `stable`
- `proc read_string` signature `proc read_string() -> string {` example `io.read_string(...)` stability `stable`
- `proc read_int` signature `proc read_int() -> i64 {` example `io.read_int(...)` stability `stable`
- `proc read_float` signature `proc read_float() -> f64 {` example `io.read_float(...)` stability `stable`
- `proc read_bool` signature `proc read_bool() -> bool {` example `io.read_bool(...)` stability `stable`
- `proc read_all_stdin` signature `proc read_all_stdin() -> string {` example `io.read_all_stdin(...)` stability `stable`
- `proc read_file` signature `proc read_file(path: string) -> string {` example `io.read_file(...)` stability `stable`
- `proc read_file_bytes` signature `proc read_file_bytes(path: string) -> bytes {` example `io.read_file_bytes(...)` stability `stable`
- `proc read_lines` signature `proc read_lines(path: string) -> [string] {` example `io.read_lines(...)` stability `stable`
- `proc write_file` signature `proc write_file(path: string, content: string) -> int {` example `io.write_file(...)` stability `stable`
- `proc write_file_bytes` signature `proc write_file_bytes(path: string, content: bytes) -> int {` example `io.write_file_bytes(...)` stability `stable`
- `proc append_file` signature `proc append_file(path: string, content: string) -> int {` example `io.append_file(...)` stability `stable`
- `proc copy_file` signature `proc copy_file(src: string, dst: string) -> int {` example `io.copy_file(...)` stability `stable`
- `proc move_file` signature `proc move_file(src: string, dst: string) -> int {` example `io.move_file(...)` stability `stable`
- `proc remove` signature `proc remove(path: string) -> int {` example `io.remove(...)` stability `stable`
- `proc rename` signature `proc rename(old_path: string, new_path: string) -> int {` example `io.rename(...)` stability `stable`
- `proc file_exists` signature `proc file_exists(path: string) -> bool {` example `io.file_exists(...)` stability `stable`
- `proc is_file` signature `proc is_file(path: string) -> bool {` example `io.is_file(...)` stability `stable`
- `proc is_dir` signature `proc is_dir(path: string) -> bool {` example `io.is_dir(...)` stability `stable`
- `proc is_symlink` signature `proc is_symlink(path: string) -> bool {` example `io.is_symlink(...)` stability `stable`
- `proc file_size` signature `proc file_size(path: string) -> i64 {` example `io.file_size(...)` stability `stable`
- `proc stat` signature `proc stat(path: string) -> FileStat {` example `io.stat(...)` stability `stable`
- `proc mkdir` signature `proc mkdir(path: string) -> int {` example `io.mkdir(...)` stability `stable`
- `proc mkdir_all` signature `proc mkdir_all(path: string) -> int {` example `io.mkdir_all(...)` stability `stable`
- `proc rmdir` signature `proc rmdir(path: string) -> int {` example `io.rmdir(...)` stability `stable`
- `proc list_dir` signature `proc list_dir(path: string) -> [DirEntry] {` example `io.list_dir(...)` stability `stable`
- `proc read_dir_names` signature `proc read_dir_names(path: string) -> [string] {` example `io.read_dir_names(...)` stability `stable`
- `proc touch` signature `proc touch(path: string) -> int {` example `io.touch(...)` stability `stable`
- `proc tmpfile` signature `proc tmpfile() -> File {` example `io.tmpfile(...)` stability `stable`
- `proc tmpnam` signature `proc tmpnam(prefix: string) -> string {` example `io.tmpnam(...)` stability `stable`
- `proc basename` signature `proc basename(path: string) -> string {` example `io.basename(...)` stability `stable`
- `proc dirname` signature `proc dirname(path: string) -> string {` example `io.dirname(...)` stability `stable`
- `proc extension` signature `proc extension(path: string) -> string {` example `io.extension(...)` stability `stable`
- `proc stem` signature `proc stem(path: string) -> string {` example `io.stem(...)` stability `stable`
- `proc path_join` signature `proc path_join(a: string, b: string) -> string {` example `io.path_join(...)` stability `stable`
- `proc path_join3` signature `proc path_join3(a: string, b: string, c: string) -> string {` example `io.path_join3(...)` stability `stable`
- `proc path_normalize` signature `proc path_normalize(path: string) -> string {` example `io.path_normalize(...)` stability `stable`
- `proc path_is_absolute` signature `proc path_is_absolute(path: string) -> bool {` example `io.path_is_absolute(...)` stability `stable`
- `proc path_is_relative` signature `proc path_is_relative(path: string) -> bool {` example `io.path_is_relative(...)` stability `stable`
- `proc path_parse` signature `proc path_parse(path: string) -> PathInfo {` example `io.path_parse(...)` stability `stable`
- `proc buffer_new` signature `proc buffer_new(capacity: usize) -> Buffer {` example `io.buffer_new(...)` stability `stable`
- `proc buffer_from_bytes` signature `proc buffer_from_bytes(data: bytes) -> Buffer {` example `io.buffer_from_bytes(...)` stability `stable`
- `proc buffer_clear` signature `proc buffer_clear(buf: Buffer) -> Buffer {` example `io.buffer_clear(...)` stability `stable`
- `proc buffer_remaining` signature `proc buffer_remaining(buf: Buffer) -> usize {` example `io.buffer_remaining(...)` stability `stable`
- `proc buffer_is_empty` signature `proc buffer_is_empty(buf: Buffer) -> bool {` example `io.buffer_is_empty(...)` stability `stable`
- `proc reader_new` signature `proc reader_new(f: File) -> TextReader {` example `io.reader_new(...)` stability `stable`
- `proc writer_new` signature `proc writer_new(f: File) -> TextWriter {` example `io.writer_new(...)` stability `stable`
- `proc reader_read_line` signature `proc reader_read_line(r: TextReader) -> string {` example `io.reader_read_line(...)` stability `stable`
- `proc writer_write` signature `proc writer_write(w: TextWriter, s: string) -> TextWriter {` example `io.writer_write(...)` stability `stable`
- `proc writer_writeln` signature `proc writer_writeln(w: TextWriter, s: string) -> TextWriter {` example `io.writer_writeln(...)` stability `stable`
- `proc perror` signature `proc perror(message: string) -> void {` example `io.perror(...)` stability `stable`
- `proc strerror` signature `proc strerror(code: int) -> string {` example `io.strerror(...)` stability `stable`
- `proc setbuf` signature `proc setbuf(f: File, buffer: bytes) -> int {` example `io.setbuf(...)` stability `stable`
- `proc setvbuf` signature `proc setvbuf(f: File, buffer: bytes, mode: i32, size: i64) -> int {` example `io.setvbuf(...)` stability `stable`
- `proc format_apply` signature `proc format_apply(format: string, args: [string]) -> string {` example `io.format_apply(...)` stability `stable`
- `proc to_string_i64` signature `proc to_string_i64(value: i64) -> string {` example `io.to_string_i64(...)` stability `stable`
- `proc to_string_f64` signature `proc to_string_f64(value: f64) -> string {` example `io.to_string_f64(...)` stability `stable`
- `proc parse_i64` signature `proc parse_i64(s: string) -> i64 {` example `io.parse_i64(...)` stability `stable`
- `proc parse_f64` signature `proc parse_f64(s: string) -> f64 {` example `io.parse_f64(...)` stability `stable`
- `proc split_lines` signature `proc split_lines(s: string) -> [string] {` example `io.split_lines(...)` stability `stable`
- `proc string_contains` signature `proc string_contains(s: string, needle: string) -> bool {` example `io.string_contains(...)` stability `stable`
- `proc string_starts_with` signature `proc string_starts_with(s: string, prefix: string) -> bool {` example `io.string_starts_with(...)` stability `stable`
- `proc string_ends_with` signature `proc string_ends_with(s: string, suffix: string) -> bool {` example `io.string_ends_with(...)` stability `stable`
- `proc string_slice` signature `proc string_slice(s: string, start: int, end: int) -> string {` example `io.string_slice(...)` stability `stable`
- `proc string_char_at` signature `proc string_char_at(s: string, index: int) -> string {` example `io.string_char_at(...)` stability `stable`
- `proc io_ready` signature `proc io_ready() -> bool {` example `io.io_ready(...)` stability `stable`
- `proc io_domains` signature `proc io_domains() -> [string] {` example `io.io_domains(...)` stability `stable`
- `proc library_meta` signature `proc library_meta() -> string {` example `io.library_meta(...)` stability `stable`
- `proc io_selftest` signature `proc io_selftest() -> bool {` example `io.io_selftest(...)` stability `stable`

## `src/vitte/stdlib/path.vitl`

Stability: `stable`

- `const PATH_SEPARATOR` signature `const PATH_SEPARATOR: string = "/"` example `path.PATH_SEPARATOR` stability `stable`
- `const PATH_DELIMITER` signature `const PATH_DELIMITER: string = ":"` example `path.PATH_DELIMITER` stability `stable`
- `form Path` signature `form Path {` example `path.Path` stability `stable`
- `form PathWalker` signature `form PathWalker {` example `path.PathWalker` stability `stable`
- `form PathLibraryManifest` signature `form PathLibraryManifest {` example `path.PathLibraryManifest` stability `stable`
- `form PathLibraryHealth` signature `form PathLibraryHealth {` example `path.PathLibraryHealth` stability `stable`
- `form PathLibrarySummary` signature `form PathLibrarySummary {` example `path.PathLibrarySummary` stability `stable`
- `form FileStat` signature `form FileStat {` example `path.FileStat` stability `stable`
- `const S_ISREG` signature `const S_ISREG: i32 = 1` example `path.S_ISREG` stability `stable`
- `const S_ISDIR` signature `const S_ISDIR: i32 = 2` example `path.S_ISDIR` stability `stable`
- `const S_ISLNK` signature `const S_ISLNK: i32 = 4` example `path.S_ISLNK` stability `stable`
- `proc listdir` signature `proc listdir(root: string) -> [string] {` example `path.listdir(...)` stability `stable`
- `proc getenv` signature `proc getenv(name: string) -> string {` example `path.getenv(...)` stability `stable`
- `proc getcwd` signature `proc getcwd() -> string {` example `path.getcwd(...)` stability `stable`
- `proc setenv` signature `proc setenv(name: string, value: string, overwrite: i32) -> i32 {` example `path.setenv(...)` stability `stable`
- `proc access` signature `proc access(p: string, mode: i32) -> i32 {` example `path.access(...)` stability `stable`
- `proc stat` signature `proc stat(p: string) -> FileStat {` example `path.stat(...)` stability `stable`
- `proc lstat` signature `proc lstat(p: string) -> FileStat {` example `path.lstat(...)` stability `stable`
- `proc _path_slice` signature `proc _path_slice(str: string, start: int, end: int) -> string {` example `path._path_slice(...)` stability `stable`
- `proc _path_strip_trailing_separator` signature `proc _path_strip_trailing_separator(p: string) -> string {` example `path._path_strip_trailing_separator(...)` stability `stable`
- `proc _path_split` signature `proc _path_split(p: string) -> [string] {` example `path._path_split(...)` stability `stable`
- `proc _path_join_parts` signature `proc _path_join_parts(parts: [string], absolute: int) -> string {` example `path._path_join_parts(...)` stability `stable`
- `proc _path_remove_last_component` signature `proc _path_remove_last_component(parts: [string]) -> [string] {` example `path._path_remove_last_component(...)` stability `stable`
- `proc _path_segment_matches` signature `proc _path_segment_matches(text: string, pattern: string) -> int {` example `path._path_segment_matches(...)` stability `stable`
- `proc _path_match_parts` signature `proc _path_match_parts(parts: [string], pattern_parts: [string], pi: int, qi: int) -> int {` example `path._path_match_parts(...)` stability `stable`
- `proc _path_collect_recursive` signature `proc _path_collect_recursive(root: string, entries: [string]) -> [string] {` example `path._path_collect_recursive(...)` stability `stable`
- `proc _join_strings` signature `proc _join_strings(values: [string], delimiter: string) -> string {` example `path._join_strings(...)` stability `stable`
- `proc _get_env_variable` signature `proc _get_env_variable(name: string) -> string {` example `path._get_env_variable(...)` stability `stable`
- `proc path_version` signature `proc path_version() -> string {` example `path.path_version(...)` stability `stable`
- `proc path_name` signature `proc path_name() -> string {` example `path.path_name(...)` stability `stable`
- `proc path_module_count` signature `proc path_module_count() -> i32 {` example `path.path_module_count(...)` stability `stable`
- `proc path_modules` signature `proc path_modules() -> [string] {` example `path.path_modules(...)` stability `stable`
- `proc path_manifest` signature `proc path_manifest() -> PathLibraryManifest {` example `path.path_manifest(...)` stability `stable`
- `proc path_ready` signature `proc path_ready() -> bool {` example `path.path_ready(...)` stability `stable`
- `proc path_health` signature `proc path_health() -> PathLibraryHealth {` example `path.path_health(...)` stability `stable`
- `proc path_summary` signature `proc path_summary() -> PathLibrarySummary {` example `path.path_summary(...)` stability `stable`
- `proc path_selftest` signature `proc path_selftest() -> bool {` example `path.path_selftest(...)` stability `stable`
- `proc path_new` signature `proc path_new(p: string) -> Path {` example `path.path_new(...)` stability `stable`
- `proc path_normalize` signature `proc path_normalize(p: string) -> string {` example `path.path_normalize(...)` stability `stable`
- `proc path_resolve` signature `proc path_resolve(p: string) -> string {` example `path.path_resolve(...)` stability `stable`
- `proc path_absolute` signature `proc path_absolute(p: string) -> string {` example `path.path_absolute(...)` stability `stable`
- `proc path_relative` signature `proc path_relative(base: string, target: string) -> string {` example `path.path_relative(...)` stability `stable`
- `proc path_get_parent` signature `proc path_get_parent(p: string) -> string {` example `path.path_get_parent(...)` stability `stable`
- `proc path_get_filename` signature `proc path_get_filename(p: string) -> string {` example `path.path_get_filename(...)` stability `stable`
- `proc path_get_basename` signature `proc path_get_basename(p: string) -> string {` example `path.path_get_basename(...)` stability `stable`
- `proc path_get_stem` signature `proc path_get_stem(p: string) -> string {` example `path.path_get_stem(...)` stability `stable`
- `proc path_get_extension` signature `proc path_get_extension(p: string) -> string {` example `path.path_get_extension(...)` stability `stable`
- `proc path_get_directory` signature `proc path_get_directory(p: string) -> string {` example `path.path_get_directory(...)` stability `stable`
- `proc path_get_root` signature `proc path_get_root(p: string) -> string {` example `path.path_get_root(...)` stability `stable`
- `proc path_join` signature `proc path_join(base: string, component: string) -> string {` example `path.path_join(...)` stability `stable`
- `proc path_join_multi` signature `proc path_join_multi(base: string, components: [string]) -> string {` example `path.path_join_multi(...)` stability `stable`
- `proc path_with_extension` signature `proc path_with_extension(p: string, ext: string) -> string {` example `path.path_with_extension(...)` stability `stable`
- `proc path_with_filename` signature `proc path_with_filename(p: string, filename: string) -> string {` example `path.path_with_filename(...)` stability `stable`
- `proc path_exists` signature `proc path_exists(p: string) -> int {` example `path.path_exists(...)` stability `stable`
- `proc path_is_file` signature `proc path_is_file(p: string) -> int {` example `path.path_is_file(...)` stability `stable`
- `proc path_is_dir` signature `proc path_is_dir(p: string) -> int {` example `path.path_is_dir(...)` stability `stable`
- `proc path_is_symlink` signature `proc path_is_symlink(p: string) -> int {` example `path.path_is_symlink(...)` stability `stable`
- `proc path_is_absolute` signature `proc path_is_absolute(p: string) -> int {` example `path.path_is_absolute(...)` stability `stable`
- `proc path_is_relative` signature `proc path_is_relative(p: string) -> int {` example `path.path_is_relative(...)` stability `stable`
- `proc path_is_hidden` signature `proc path_is_hidden(p: string) -> int {` example `path.path_is_hidden(...)` stability `stable`
- `proc path_equals` signature `proc path_equals(p1: string, p2: string) -> int {` example `path.path_equals(...)` stability `stable`
- `proc path_starts_with` signature `proc path_starts_with(p: string, prefix: string) -> int {` example `path.path_starts_with(...)` stability `stable`
- `proc path_ends_with` signature `proc path_ends_with(p: string, suffix: string) -> int {` example `path.path_ends_with(...)` stability `stable`
- `proc path_matches` signature `proc path_matches(p: string, pattern: string) -> int {` example `path.path_matches(...)` stability `stable`
- `proc path_glob` signature `proc path_glob(pattern: string) -> [string] {` example `path.path_glob(...)` stability `stable`
- `proc path_glob_recursive` signature `proc path_glob_recursive(pattern: string) -> [string] {` example `path.path_glob_recursive(...)` stability `stable`
- `proc path_walk` signature `proc path_walk(root: string) -> PathWalker {` example `path.path_walk(...)` stability `stable`
- `proc path_walk_recursive` signature `proc path_walk_recursive(root: string) -> PathWalker {` example `path.path_walk_recursive(...)` stability `stable`
- `proc path_walker_next` signature `proc path_walker_next(w: PathWalker) -> string {` example `path.path_walker_next(...)` stability `stable`
- `proc path_walker_has_next` signature `proc path_walker_has_next(w: PathWalker) -> int {` example `path.path_walker_has_next(...)` stability `stable`
- `proc path_clean` signature `proc path_clean(p: string) -> string {` example `path.path_clean(...)` stability `stable`
- `proc path_canonicalize` signature `proc path_canonicalize(p: string) -> string {` example `path.path_canonicalize(...)` stability `stable`
- `proc path_simplify` signature `proc path_simplify(p: string) -> string {` example `path.path_simplify(...)` stability `stable`
- `proc path_expand_home` signature `proc path_expand_home(p: string) -> string {` example `path.path_expand_home(...)` stability `stable`
- `proc path_expand_env` signature `proc path_expand_env(p: string) -> string {` example `path.path_expand_env(...)` stability `stable`
- `proc home_dir` signature `proc home_dir() -> string {` example `path.home_dir(...)` stability `stable`
- `proc temp_dir` signature `proc temp_dir() -> string {` example `path.temp_dir(...)` stability `stable`
- `proc current_dir` signature `proc current_dir() -> string {` example `path.current_dir(...)` stability `stable`
- `proc config_dir` signature `proc config_dir() -> string {` example `path.config_dir(...)` stability `stable`
- `proc cache_dir` signature `proc cache_dir() -> string {` example `path.cache_dir(...)` stability `stable`
- `proc desktop_dir` signature `proc desktop_dir() -> string {` example `path.desktop_dir(...)` stability `stable`
- `proc documents_dir` signature `proc documents_dir() -> string {` example `path.documents_dir(...)` stability `stable`
- `proc downloads_dir` signature `proc downloads_dir() -> string {` example `path.downloads_dir(...)` stability `stable`
- `proc get_path_env` signature `proc get_path_env() -> [string] {` example `path.get_path_env(...)` stability `stable`
- `proc find_in_path` signature `proc find_in_path(executable: string) -> string {` example `path.find_in_path(...)` stability `stable`
- `proc add_to_path` signature `proc add_to_path(directory: string) -> int {` example `path.add_to_path(...)` stability `stable`
- `proc remove_from_path` signature `proc remove_from_path(directory: string) -> int {` example `path.remove_from_path(...)` stability `stable`
- `proc path_segments` signature `proc path_segments(p: string) -> [string] {` example `path.path_segments(...)` stability `stable`
- `proc path_depth` signature `proc path_depth(p: string) -> i32 {` example `path.path_depth(...)` stability `stable`
- `proc path_report` signature `proc path_report(p: string) -> Path {` example `path.path_report(...)` stability `stable`

## `src/vitte/stdlib/os.vitl`

Stability: `stable`

- `const OS_VERSION` signature `const OS_VERSION: string = ""` example `os.OS_VERSION` stability `stable`
- `const OK` signature `const OK: int = 0` example `os.OK` stability `stable`
- `const ERR` signature `const ERR: int = 0` example `os.ERR` stability `stable`
- `const EOF` signature `const EOF: int = 0` example `os.EOF` stability `stable`
- `const EXIT_SUCCESS` signature `const EXIT_SUCCESS: int = 0` example `os.EXIT_SUCCESS` stability `stable`
- `const EXIT_FAILURE` signature `const EXIT_FAILURE: int = 0` example `os.EXIT_FAILURE` stability `stable`
- `const STDIN_FILENO` signature `const STDIN_FILENO: int = 0` example `os.STDIN_FILENO` stability `stable`
- `const STDOUT_FILENO` signature `const STDOUT_FILENO: int = 0` example `os.STDOUT_FILENO` stability `stable`
- `const STDERR_FILENO` signature `const STDERR_FILENO: int = 0` example `os.STDERR_FILENO` stability `stable`
- `const SEEK_SET` signature `const SEEK_SET: int = 0` example `os.SEEK_SET` stability `stable`
- `const SEEK_CUR` signature `const SEEK_CUR: int = 0` example `os.SEEK_CUR` stability `stable`
- `const SEEK_END` signature `const SEEK_END: int = 0` example `os.SEEK_END` stability `stable`
- `const AT_FDCWD` signature `const AT_FDCWD: int = 0` example `os.AT_FDCWD` stability `stable`
- `const F_OK` signature `const F_OK: int = 0` example `os.F_OK` stability `stable`
- `const X_OK` signature `const X_OK: int = 0` example `os.X_OK` stability `stable`
- `const W_OK` signature `const W_OK: int = 0` example `os.W_OK` stability `stable`
- `const R_OK` signature `const R_OK: int = 0` example `os.R_OK` stability `stable`
- `const O_RDONLY` signature `const O_RDONLY: int = 0` example `os.O_RDONLY` stability `stable`
- `const O_WRONLY` signature `const O_WRONLY: int = 0` example `os.O_WRONLY` stability `stable`
- `const O_RDWR` signature `const O_RDWR: int = 0` example `os.O_RDWR` stability `stable`
- `const O_CREAT` signature `const O_CREAT: int = 0` example `os.O_CREAT` stability `stable`
- `const O_EXCL` signature `const O_EXCL: int = 0` example `os.O_EXCL` stability `stable`
- `const O_NOCTTY` signature `const O_NOCTTY: int = 0` example `os.O_NOCTTY` stability `stable`
- `const O_TRUNC` signature `const O_TRUNC: int = 0` example `os.O_TRUNC` stability `stable`
- `const O_APPEND` signature `const O_APPEND: int = 0` example `os.O_APPEND` stability `stable`
- `const O_NONBLOCK` signature `const O_NONBLOCK: int = 0` example `os.O_NONBLOCK` stability `stable`
- `const O_DIRECTORY` signature `const O_DIRECTORY: int = 0` example `os.O_DIRECTORY` stability `stable`
- `const O_NOFOLLOW` signature `const O_NOFOLLOW: int = 0` example `os.O_NOFOLLOW` stability `stable`
- `const O_CLOEXEC` signature `const O_CLOEXEC: int = 0` example `os.O_CLOEXEC` stability `stable`
- `const S_IFMT` signature `const S_IFMT: int = 0` example `os.S_IFMT` stability `stable`
- `const S_IFSOCK` signature `const S_IFSOCK: int = 0` example `os.S_IFSOCK` stability `stable`
- `const S_IFLNK` signature `const S_IFLNK: int = 0` example `os.S_IFLNK` stability `stable`
- `const S_IFREG` signature `const S_IFREG: int = 0` example `os.S_IFREG` stability `stable`
- `const S_IFBLK` signature `const S_IFBLK: int = 0` example `os.S_IFBLK` stability `stable`
- `const S_IFDIR` signature `const S_IFDIR: int = 0` example `os.S_IFDIR` stability `stable`
- `const S_IFCHR` signature `const S_IFCHR: int = 0` example `os.S_IFCHR` stability `stable`
- `const S_IFIFO` signature `const S_IFIFO: int = 0` example `os.S_IFIFO` stability `stable`
- `const S_IRUSR` signature `const S_IRUSR: int = 0` example `os.S_IRUSR` stability `stable`
- `const S_IWUSR` signature `const S_IWUSR: int = 0` example `os.S_IWUSR` stability `stable`
- `const S_IXUSR` signature `const S_IXUSR: int = 0` example `os.S_IXUSR` stability `stable`
- `const S_IRGRP` signature `const S_IRGRP: int = 0` example `os.S_IRGRP` stability `stable`
- `const S_IWGRP` signature `const S_IWGRP: int = 0` example `os.S_IWGRP` stability `stable`
- `const S_IXGRP` signature `const S_IXGRP: int = 0` example `os.S_IXGRP` stability `stable`
- `const S_IROTH` signature `const S_IROTH: int = 0` example `os.S_IROTH` stability `stable`
- `const S_IWOTH` signature `const S_IWOTH: int = 0` example `os.S_IWOTH` stability `stable`
- `const S_IXOTH` signature `const S_IXOTH: int = 0` example `os.S_IXOTH` stability `stable`
- `const SIGINT` signature `const SIGINT: int = 0` example `os.SIGINT` stability `stable`
- `const SIGQUIT` signature `const SIGQUIT: int = 0` example `os.SIGQUIT` stability `stable`
- `const SIGILL` signature `const SIGILL: int = 0` example `os.SIGILL` stability `stable`
- `const SIGABRT` signature `const SIGABRT: int = 0` example `os.SIGABRT` stability `stable`
- `const SIGFPE` signature `const SIGFPE: int = 0` example `os.SIGFPE` stability `stable`
- `const SIGKILL` signature `const SIGKILL: int = 0` example `os.SIGKILL` stability `stable`
- `const SIGSEGV` signature `const SIGSEGV: int = 0` example `os.SIGSEGV` stability `stable`
- `const SIGPIPE` signature `const SIGPIPE: int = 0` example `os.SIGPIPE` stability `stable`
- `const SIGALRM` signature `const SIGALRM: int = 0` example `os.SIGALRM` stability `stable`
- `const SIGTERM` signature `const SIGTERM: int = 0` example `os.SIGTERM` stability `stable`
- `const SIGCHLD` signature `const SIGCHLD: int = 0` example `os.SIGCHLD` stability `stable`
- `const SIGCONT` signature `const SIGCONT: int = 0` example `os.SIGCONT` stability `stable`
- `const SIGSTOP` signature `const SIGSTOP: int = 0` example `os.SIGSTOP` stability `stable`
- `const SIGTSTP` signature `const SIGTSTP: int = 0` example `os.SIGTSTP` stability `stable`
- `const SIGTTIN` signature `const SIGTTIN: int = 0` example `os.SIGTTIN` stability `stable`
- `const SIGTTOU` signature `const SIGTTOU: int = 0` example `os.SIGTTOU` stability `stable`
- `const WNOHANG` signature `const WNOHANG: int = 0` example `os.WNOHANG` stability `stable`
- `const WUNTRACED` signature `const WUNTRACED: int = 0` example `os.WUNTRACED` stability `stable`
- `const WCONTINUED` signature `const WCONTINUED: int = 0` example `os.WCONTINUED` stability `stable`
- `const EPERM` signature `const EPERM: int = 0` example `os.EPERM` stability `stable`
- `const ENOENT` signature `const ENOENT: int = 0` example `os.ENOENT` stability `stable`
- `const ESRCH` signature `const ESRCH: int = 0` example `os.ESRCH` stability `stable`
- `const EINTR` signature `const EINTR: int = 0` example `os.EINTR` stability `stable`
- `const EIO` signature `const EIO: int = 0` example `os.EIO` stability `stable`
- `const ENXIO` signature `const ENXIO: int = 0` example `os.ENXIO` stability `stable`
- `const E2BIG` signature `const E2BIG: int = 0` example `os.E2BIG` stability `stable`
- `const ENOEXEC` signature `const ENOEXEC: int = 0` example `os.ENOEXEC` stability `stable`
- `const EBADF` signature `const EBADF: int = 0` example `os.EBADF` stability `stable`
- `const ECHILD` signature `const ECHILD: int = 0` example `os.ECHILD` stability `stable`
- `const EAGAIN` signature `const EAGAIN: int = 0` example `os.EAGAIN` stability `stable`
- `const ENOMEM` signature `const ENOMEM: int = 0` example `os.ENOMEM` stability `stable`
- `const EACCES` signature `const EACCES: int = 0` example `os.EACCES` stability `stable`
- `const EFAULT` signature `const EFAULT: int = 0` example `os.EFAULT` stability `stable`
- `const EBUSY` signature `const EBUSY: int = 0` example `os.EBUSY` stability `stable`
- `const EEXIST` signature `const EEXIST: int = 0` example `os.EEXIST` stability `stable`
- `const EXDEV` signature `const EXDEV: int = 0` example `os.EXDEV` stability `stable`
- `const ENODEV` signature `const ENODEV: int = 0` example `os.ENODEV` stability `stable`
- `const ENOTDIR` signature `const ENOTDIR: int = 0` example `os.ENOTDIR` stability `stable`
- `const EISDIR` signature `const EISDIR: int = 0` example `os.EISDIR` stability `stable`
- `const EINVAL` signature `const EINVAL: int = 0` example `os.EINVAL` stability `stable`
- `const ENFILE` signature `const ENFILE: int = 0` example `os.ENFILE` stability `stable`
- `const EMFILE` signature `const EMFILE: int = 0` example `os.EMFILE` stability `stable`
- `const ENOTTY` signature `const ENOTTY: int = 0` example `os.ENOTTY` stability `stable`
- `const EFBIG` signature `const EFBIG: int = 0` example `os.EFBIG` stability `stable`
- `const ENOSPC` signature `const ENOSPC: int = 0` example `os.ENOSPC` stability `stable`
- `const ESPIPE` signature `const ESPIPE: int = 0` example `os.ESPIPE` stability `stable`
- `const EROFS` signature `const EROFS: int = 0` example `os.EROFS` stability `stable`
- `const EMLINK` signature `const EMLINK: int = 0` example `os.EMLINK` stability `stable`
- `const EPIPE` signature `const EPIPE: int = 0` example `os.EPIPE` stability `stable`
- `const ERANGE` signature `const ERANGE: int = 0` example `os.ERANGE` stability `stable`
- `const ENOSYS` signature `const ENOSYS: int = 0` example `os.ENOSYS` stability `stable`
- `const ENOTEMPTY` signature `const ENOTEMPTY: int = 0` example `os.ENOTEMPTY` stability `stable`
- `const ELOOP` signature `const ELOOP: int = 0` example `os.ELOOP` stability `stable`
- `const ENAMETOOLONG` signature `const ENAMETOOLONG: int = 0` example `os.ENAMETOOLONG` stability `stable`
- `const ETIMEDOUT` signature `const ETIMEDOUT: int = 0` example `os.ETIMEDOUT` stability `stable`
- `const ECONNREFUSED` signature `const ECONNREFUSED: int = 0` example `os.ECONNREFUSED` stability `stable`
- `const ENOTSUP` signature `const ENOTSUP: int = 0` example `os.ENOTSUP` stability `stable`
- `pick OsStatus` signature `pick OsStatus {` example `os.OsStatus` stability `stable`
- `pick PlatformKind` signature `pick PlatformKind {` example `os.PlatformKind` stability `stable`
- `pick ProcessState` signature `pick ProcessState {` example `os.ProcessState` stability `stable`
- `pick FileType` signature `pick FileType {` example `os.FileType` stability `stable`
- `pick OpenMode` signature `pick OpenMode {` example `os.OpenMode` stability `stable`
- `pick SignalDisposition` signature `pick SignalDisposition {` example `os.SignalDisposition` stability `stable`
- `form OsError` signature `form OsError {` example `os.OsError` stability `stable`
- `form OsResult` signature `form OsResult {` example `os.OsResult` stability `stable`
- `form Process` signature `form Process {` example `os.Process` stability `stable`
- `form ProcessInfo` signature `form ProcessInfo {` example `os.ProcessInfo` stability `stable`
- `form SpawnOptions` signature `form SpawnOptions {` example `os.SpawnOptions` stability `stable`
- `form ExecResult` signature `form ExecResult {` example `os.ExecResult` stability `stable`
- `form EnvVar` signature `form EnvVar {` example `os.EnvVar` stability `stable`
- `form User` signature `form User {` example `os.User` stability `stable`
- `form Group` signature `form Group {` example `os.Group` stability `stable`
- `form SystemInfo` signature `form SystemInfo {` example `os.SystemInfo` stability `stable`
- `form FileDescriptor` signature `form FileDescriptor {` example `os.FileDescriptor` stability `stable`
- `form FileStat` signature `form FileStat {` example `os.FileStat` stability `stable`
- `form DirEntry` signature `form DirEntry {` example `os.DirEntry` stability `stable`
- `form PathInfo` signature `form PathInfo {` example `os.PathInfo` stability `stable`
- `form Pipe` signature `form Pipe {` example `os.Pipe` stability `stable`
- `form WaitStatus` signature `form WaitStatus {` example `os.WaitStatus` stability `stable`
- `form SignalAction` signature `form SignalAction {` example `os.SignalAction` stability `stable`
- `form Timespec` signature `form Timespec {` example `os.Timespec` stability `stable`
- `form Timeval` signature `form Timeval {` example `os.Timeval` stability `stable`
- `form ResourceUsage` signature `form ResourceUsage {` example `os.ResourceUsage` stability `stable`
- `form MountInfo` signature `form MountInfo {` example `os.MountInfo` stability `stable`
- `form TerminalSize` signature `form TerminalSize {` example `os.TerminalSize` stability `stable`
- `form PollFd` signature `form PollFd {` example `os.PollFd` stability `stable`
- `form OsSnapshot` signature `form OsSnapshot {` example `os.OsSnapshot` stability `stable`
- `proc os_result_ok` signature `proc os_result_ok() -> int {` example `os.os_result_ok(...)` stability `stable`
- `proc os_result_error` signature `proc os_result_error() -> int {` example `os.os_result_error(...)` stability `stable`
- `proc os_error` signature `proc os_error() -> int {` example `os.os_error(...)` stability `stable`
- `proc errno_name` signature `proc errno_name() -> int {` example `os.errno_name(...)` stability `stable`
- `proc strerror` signature `proc strerror() -> int {` example `os.strerror(...)` stability `stable`
- `proc status_from_errno` signature `proc status_from_errno() -> int {` example `os.status_from_errno(...)` stability `stable`
- `proc getpid` signature `proc getpid() -> int {` example `os.getpid(...)` stability `stable`
- `proc getppid` signature `proc getppid() -> int {` example `os.getppid(...)` stability `stable`
- `proc getpgid` signature `proc getpgid() -> int {` example `os.getpgid(...)` stability `stable`
- `proc getsid` signature `proc getsid() -> int {` example `os.getsid(...)` stability `stable`
- `proc setpgid` signature `proc setpgid() -> int {` example `os.setpgid(...)` stability `stable`
- `proc fork` signature `proc fork() -> int {` example `os.fork(...)` stability `stable`
- `proc vfork` signature `proc vfork() -> int {` example `os.vfork(...)` stability `stable`
- `proc exec` signature `proc exec() -> int {` example `os.exec(...)` stability `stable`
- `proc execv` signature `proc execv() -> int {` example `os.execv(...)` stability `stable`
- `proc execve` signature `proc execve() -> int {` example `os.execve(...)` stability `stable`
- `proc execvp` signature `proc execvp() -> int {` example `os.execvp(...)` stability `stable`
- `proc system` signature `proc system() -> int {` example `os.system(...)` stability `stable`
- `proc spawn` signature `proc spawn() -> int {` example `os.spawn(...)` stability `stable`
- `proc spawn_with_options` signature `proc spawn_with_options() -> int {` example `os.spawn_with_options(...)` stability `stable`
- `proc run` signature `proc run() -> int {` example `os.run(...)` stability `stable`
- `proc wait` signature `proc wait() -> int {` example `os.wait(...)` stability `stable`
- `proc waitpid` signature `proc waitpid() -> int {` example `os.waitpid(...)` stability `stable`
- `proc wait_status_code` signature `proc wait_status_code() -> int {` example `os.wait_status_code(...)` stability `stable`
- `proc kill` signature `proc kill() -> int {` example `os.kill(...)` stability `stable`
- `proc signal` signature `proc signal() -> int {` example `os.signal(...)` stability `stable`
- `proc signal_ignore` signature `proc signal_ignore() -> int {` example `os.signal_ignore(...)` stability `stable`
- `proc signal_default` signature `proc signal_default() -> int {` example `os.signal_default(...)` stability `stable`
- `proc raise` signature `proc raise() -> int {` example `os.raise(...)` stability `stable`
- `proc pause` signature `proc pause() -> int {` example `os.pause(...)` stability `stable`
- `proc exit` signature `proc exit() -> int {` example `os.exit(...)` stability `stable`
- `proc abort` signature `proc abort() -> int {` example `os.abort(...)` stability `stable`
- `proc process_current` signature `proc process_current() -> int {` example `os.process_current(...)` stability `stable`
- `proc process_info` signature `proc process_info() -> int {` example `os.process_info(...)` stability `stable`
- `proc process_list` signature `proc process_list() -> int {` example `os.process_list(...)` stability `stable`
- `proc process_exists` signature `proc process_exists() -> int {` example `os.process_exists(...)` stability `stable`
- `proc process_kill` signature `proc process_kill() -> int {` example `os.process_kill(...)` stability `stable`
- `proc getuid` signature `proc getuid() -> int {` example `os.getuid(...)` stability `stable`
- `proc geteuid` signature `proc geteuid() -> int {` example `os.geteuid(...)` stability `stable`
- `proc getgid` signature `proc getgid() -> int {` example `os.getgid(...)` stability `stable`
- `proc getegid` signature `proc getegid() -> int {` example `os.getegid(...)` stability `stable`
- `proc setuid` signature `proc setuid() -> int {` example `os.setuid(...)` stability `stable`
- `proc setgid` signature `proc setgid() -> int {` example `os.setgid(...)` stability `stable`
- `proc getuser` signature `proc getuser() -> int {` example `os.getuser(...)` stability `stable`
- `proc getgroup` signature `proc getgroup() -> int {` example `os.getgroup(...)` stability `stable`
- `proc getenv` signature `proc getenv() -> int {` example `os.getenv(...)` stability `stable`
- `proc getenv_or` signature `proc getenv_or() -> int {` example `os.getenv_or(...)` stability `stable`
- `proc setenv` signature `proc setenv() -> int {` example `os.setenv(...)` stability `stable`
- `proc unsetenv` signature `proc unsetenv() -> int {` example `os.unsetenv(...)` stability `stable`
- `proc hasenv` signature `proc hasenv() -> int {` example `os.hasenv(...)` stability `stable`
- `proc env_list` signature `proc env_list() -> int {` example `os.env_list(...)` stability `stable`
- `proc env_get_all` signature `proc env_get_all() -> int {` example `os.env_get_all(...)` stability `stable`
- `proc env_clear` signature `proc env_clear() -> int {` example `os.env_clear(...)` stability `stable`
- `proc env_find` signature `proc env_find() -> int {` example `os.env_find(...)` stability `stable`
- `proc env_set_local` signature `proc env_set_local() -> int {` example `os.env_set_local(...)` stability `stable`
- `proc chdir` signature `proc chdir() -> int {` example `os.chdir(...)` stability `stable`
- `proc getcwd` signature `proc getcwd() -> int {` example `os.getcwd(...)` stability `stable`
- `proc cwd` signature `proc cwd() -> int {` example `os.cwd(...)` stability `stable`
- `proc gethostname` signature `proc gethostname() -> int {` example `os.gethostname(...)` stability `stable`
- `proc sethostname` signature `proc sethostname() -> int {` example `os.sethostname(...)` stability `stable`
- `proc uname` signature `proc uname() -> int {` example `os.uname(...)` stability `stable`
- `proc system_info` signature `proc system_info() -> int {` example `os.system_info(...)` stability `stable`
- `proc os_name` signature `proc os_name() -> int {` example `os.os_name(...)` stability `stable`
- `proc os_version` signature `proc os_version() -> int {` example `os.os_version(...)` stability `stable`
- `proc kernel_version` signature `proc kernel_version() -> int {` example `os.kernel_version(...)` stability `stable`
- `proc arch_name` signature `proc arch_name() -> int {` example `os.arch_name(...)` stability `stable`
- `proc platform_kind` signature `proc platform_kind() -> int {` example `os.platform_kind(...)` stability `stable`
- `proc is_unix` signature `proc is_unix() -> int {` example `os.is_unix(...)` stability `stable`
- `proc is_windows` signature `proc is_windows() -> int {` example `os.is_windows(...)` stability `stable`
- `proc is_vitte_os` signature `proc is_vitte_os() -> int {` example `os.is_vitte_os(...)` stability `stable`
- `proc is_kernel` signature `proc is_kernel() -> int {` example `os.is_kernel(...)` stability `stable`
- `proc cpu_count` signature `proc cpu_count() -> int {` example `os.cpu_count(...)` stability `stable`
- `proc page_size` signature `proc page_size() -> int {` example `os.page_size(...)` stability `stable`
- `proc uptime` signature `proc uptime() -> int {` example `os.uptime(...)` stability `stable`
- `proc clock_time` signature `proc clock_time() -> int {` example `os.clock_time(...)` stability `stable`
- `proc time_now` signature `proc time_now() -> int {` example `os.time_now(...)` stability `stable`
- `proc gettimeofday` signature `proc gettimeofday() -> int {` example `os.gettimeofday(...)` stability `stable`
- `proc timespec` signature `proc timespec() -> int {` example `os.timespec(...)` stability `stable`
- `proc timeval` signature `proc timeval() -> int {` example `os.timeval(...)` stability `stable`
- `proc sleep` signature `proc sleep() -> int {` example `os.sleep(...)` stability `stable`
- `proc usleep` signature `proc usleep() -> int {` example `os.usleep(...)` stability `stable`
- `proc nanosleep` signature `proc nanosleep() -> int {` example `os.nanosleep(...)` stability `stable`
- `proc open` signature `proc open() -> int {` example `os.open(...)` stability `stable`
- `proc open_mode` signature `proc open_mode() -> int {` example `os.open_mode(...)` stability `stable`
- `proc close` signature `proc close() -> int {` example `os.close(...)` stability `stable`
- `proc read` signature `proc read() -> int {` example `os.read(...)` stability `stable`
- `proc write` signature `proc write() -> int {` example `os.write(...)` stability `stable`
- `proc pread` signature `proc pread() -> int {` example `os.pread(...)` stability `stable`
- `proc pwrite` signature `proc pwrite() -> int {` example `os.pwrite(...)` stability `stable`
- `proc lseek` signature `proc lseek() -> int {` example `os.lseek(...)` stability `stable`
- `proc dup` signature `proc dup() -> int {` example `os.dup(...)` stability `stable`
- `proc dup2` signature `proc dup2() -> int {` example `os.dup2(...)` stability `stable`
- `proc pipe` signature `proc pipe() -> int {` example `os.pipe(...)` stability `stable`
- `proc pipe2` signature `proc pipe2() -> int {` example `os.pipe2(...)` stability `stable`
- `proc poll` signature `proc poll() -> int {` example `os.poll(...)` stability `stable`
- `proc os_select` signature `proc os_select() -> int {` example `os.os_select(...)` stability `stable`
- `proc file_descriptor` signature `proc file_descriptor() -> int {` example `os.file_descriptor(...)` stability `stable`
- `proc fd_valid` signature `proc fd_valid() -> int {` example `os.fd_valid(...)` stability `stable`
- `proc fd_is_standard` signature `proc fd_is_standard() -> int {` example `os.fd_is_standard(...)` stability `stable`
- `proc empty_stat` signature `proc empty_stat() -> int {` example `os.empty_stat(...)` stability `stable`
- `proc stat` signature `proc stat() -> int {` example `os.stat(...)` stability `stable`
- `proc lstat` signature `proc lstat() -> int {` example `os.lstat(...)` stability `stable`
- `proc fstat` signature `proc fstat() -> int {` example `os.fstat(...)` stability `stable`
- `proc mode_is_file` signature `proc mode_is_file() -> int {` example `os.mode_is_file(...)` stability `stable`
- `proc mode_is_dir` signature `proc mode_is_dir() -> int {` example `os.mode_is_dir(...)` stability `stable`
- `proc mode_is_symlink` signature `proc mode_is_symlink() -> int {` example `os.mode_is_symlink(...)` stability `stable`
- `proc exists` signature `proc exists() -> int {` example `os.exists(...)` stability `stable`
- `proc is_file` signature `proc is_file() -> int {` example `os.is_file(...)` stability `stable`
- `proc is_dir` signature `proc is_dir() -> int {` example `os.is_dir(...)` stability `stable`
- `proc is_symlink` signature `proc is_symlink() -> int {` example `os.is_symlink(...)` stability `stable`
- `proc file_size` signature `proc file_size() -> int {` example `os.file_size(...)` stability `stable`
- `proc chmod` signature `proc chmod() -> int {` example `os.chmod(...)` stability `stable`
- `proc chown` signature `proc chown() -> int {` example `os.chown(...)` stability `stable`
- `proc mkdir` signature `proc mkdir() -> int {` example `os.mkdir(...)` stability `stable`
- `proc mkdir_all` signature `proc mkdir_all() -> int {` example `os.mkdir_all(...)` stability `stable`
- `proc rmdir` signature `proc rmdir() -> int {` example `os.rmdir(...)` stability `stable`
- `proc unlink` signature `proc unlink() -> int {` example `os.unlink(...)` stability `stable`
- `proc remove` signature `proc remove() -> int {` example `os.remove(...)` stability `stable`
- `proc rename` signature `proc rename() -> int {` example `os.rename(...)` stability `stable`
- `proc link` signature `proc link() -> int {` example `os.link(...)` stability `stable`
- `proc symlink` signature `proc symlink() -> int {` example `os.symlink(...)` stability `stable`
- `proc readlink` signature `proc readlink() -> int {` example `os.readlink(...)` stability `stable`
- `proc truncate` signature `proc truncate() -> int {` example `os.truncate(...)` stability `stable`
- `proc ftruncate` signature `proc ftruncate() -> int {` example `os.ftruncate(...)` stability `stable`
- `proc access` signature `proc access() -> int {` example `os.access(...)` stability `stable`
- `proc list_dir` signature `proc list_dir() -> int {` example `os.list_dir(...)` stability `stable`
- `proc dir_names` signature `proc dir_names() -> int {` example `os.dir_names(...)` stability `stable`
- `proc mount` signature `proc mount() -> int {` example `os.mount(...)` stability `stable`
- `proc umount` signature `proc umount() -> int {` example `os.umount(...)` stability `stable`
- `proc mounts` signature `proc mounts() -> int {` example `os.mounts(...)` stability `stable`
- `proc path_sep` signature `proc path_sep() -> int {` example `os.path_sep(...)` stability `stable`
- `proc path_join` signature `proc path_join() -> int {` example `os.path_join(...)` stability `stable`
- `proc path_join3` signature `proc path_join3() -> int {` example `os.path_join3(...)` stability `stable`
- `proc path_is_absolute` signature `proc path_is_absolute() -> int {` example `os.path_is_absolute(...)` stability `stable`
- `proc path_is_relative` signature `proc path_is_relative() -> int {` example `os.path_is_relative(...)` stability `stable`
- `proc path_normalize` signature `proc path_normalize() -> int {` example `os.path_normalize(...)` stability `stable`
- `proc basename` signature `proc basename() -> int {` example `os.basename(...)` stability `stable`
- `proc dirname` signature `proc dirname() -> int {` example `os.dirname(...)` stability `stable`
- `proc extension` signature `proc extension() -> int {` example `os.extension(...)` stability `stable`
- `proc stem` signature `proc stem() -> int {` example `os.stem(...)` stability `stable`
- `proc path_parse` signature `proc path_parse() -> int {` example `os.path_parse(...)` stability `stable`
- `proc tmpdir` signature `proc tmpdir() -> int {` example `os.tmpdir(...)` stability `stable`
- `proc tmpnam` signature `proc tmpnam() -> int {` example `os.tmpnam(...)` stability `stable`
- `proc mktemp` signature `proc mktemp() -> int {` example `os.mktemp(...)` stability `stable`
- `proc mkstemp` signature `proc mkstemp() -> int {` example `os.mkstemp(...)` stability `stable`
- `proc terminal_size` signature `proc terminal_size() -> int {` example `os.terminal_size(...)` stability `stable`
- `proc isatty` signature `proc isatty() -> int {` example `os.isatty(...)` stability `stable`
- `proc ttyname` signature `proc ttyname() -> int {` example `os.ttyname(...)` stability `stable`
- `proc resource_usage` signature `proc resource_usage() -> int {` example `os.resource_usage(...)` stability `stable`
- `proc getrusage` signature `proc getrusage() -> int {` example `os.getrusage(...)` stability `stable`
- `proc reboot` signature `proc reboot() -> int {` example `os.reboot(...)` stability `stable`
- `proc shutdown` signature `proc shutdown() -> int {` example `os.shutdown(...)` stability `stable`
- `proc panic_os` signature `proc panic_os() -> int {` example `os.panic_os(...)` stability `stable`
- `proc syscall0` signature `proc syscall0() -> int {` example `os.syscall0(...)` stability `stable`
- `proc syscall1` signature `proc syscall1() -> int {` example `os.syscall1(...)` stability `stable`
- `proc syscall2` signature `proc syscall2() -> int {` example `os.syscall2(...)` stability `stable`
- `proc syscall3` signature `proc syscall3() -> int {` example `os.syscall3(...)` stability `stable`
- `proc syscall4` signature `proc syscall4() -> int {` example `os.syscall4(...)` stability `stable`
- `proc syscall5` signature `proc syscall5() -> int {` example `os.syscall5(...)` stability `stable`
- `proc syscall6` signature `proc syscall6() -> int {` example `os.syscall6(...)` stability `stable`
- `proc os_snapshot` signature `proc os_snapshot() -> int {` example `os.os_snapshot(...)` stability `stable`
- `proc string_starts_with` signature `proc string_starts_with() -> int {` example `os.string_starts_with(...)` stability `stable`
- `proc string_ends_with` signature `proc string_ends_with() -> int {` example `os.string_ends_with(...)` stability `stable`
- `proc os_domains` signature `proc os_domains() -> int {` example `os.os_domains(...)` stability `stable`
- `proc os_ready` signature `proc os_ready() -> int {` example `os.os_ready(...)` stability `stable`
- `proc library_meta` signature `proc library_meta() -> int {` example `os.library_meta(...)` stability `stable`
- `proc os_selftest` signature `proc os_selftest() -> int {` example `os.os_selftest(...)` stability `stable`

## `src/vitte/stdlib/sysinfo.vitl`

Stability: `stable`

- `form OSInfo` signature `form OSInfo {` example `sysinfo.OSInfo` stability `stable`
- `form CPUInfo` signature `form CPUInfo {` example `sysinfo.CPUInfo` stability `stable`
- `form MemoryInfo` signature `form MemoryInfo {` example `sysinfo.MemoryInfo` stability `stable`
- `form DiskStat` signature `form DiskStat {` example `sysinfo.DiskStat` stability `stable`
- `form ProcessInfo` signature `form ProcessInfo {` example `sysinfo.ProcessInfo` stability `stable`
- `form NetworkStat` signature `form NetworkStat {` example `sysinfo.NetworkStat` stability `stable`
- `form BatteryInfo` signature `form BatteryInfo {` example `sysinfo.BatteryInfo` stability `stable`
- `form SystemLoad` signature `form SystemLoad {` example `sysinfo.SystemLoad` stability `stable`
- `form SysInfoManifest` signature `form SysInfoManifest {` example `sysinfo.SysInfoManifest` stability `stable`
- `form SystemSummary` signature `form SystemSummary {` example `sysinfo.SystemSummary` stability `stable`
- `form SystemSnapshot` signature `form SystemSnapshot {` example `sysinfo.SystemSnapshot` stability `stable`
- `form SystemHealthReport` signature `form SystemHealthReport {` example `sysinfo.SystemHealthReport` stability `stable`
- `form SysInfoReport` signature `form SysInfoReport {` example `sysinfo.SysInfoReport` stability `stable`
- `proc sysinfo_version` signature `proc sysinfo_version() -> string {` example `sysinfo.sysinfo_version(...)` stability `stable`
- `proc sysinfo_name` signature `proc sysinfo_name() -> string {` example `sysinfo.sysinfo_name(...)` stability `stable`
- `proc sysinfo_module_count` signature `proc sysinfo_module_count() -> i32 {` example `sysinfo.sysinfo_module_count(...)` stability `stable`
- `proc sysinfo_modules` signature `proc sysinfo_modules() -> [string] {` example `sysinfo.sysinfo_modules(...)` stability `stable`
- `proc sysinfo_manifest` signature `proc sysinfo_manifest() -> SysInfoManifest {` example `sysinfo.sysinfo_manifest(...)` stability `stable`
- `proc sysinfo_ready` signature `proc sysinfo_ready() -> bool {` example `sysinfo.sysinfo_ready(...)` stability `stable`
- `proc sysinfo_modules_detail` signature `proc sysinfo_modules_detail() -> [string] {` example `sysinfo.sysinfo_modules_detail(...)` stability `stable`
- `proc sysinfo_collection_count` signature `proc sysinfo_collection_count() -> i32 {` example `sysinfo.sysinfo_collection_count(...)` stability `stable`
- `proc get_os_info` signature `proc get_os_info() -> OSInfo {` example `sysinfo.get_os_info(...)` stability `stable`
- `proc get_os_name` signature `proc get_os_name() -> string {` example `sysinfo.get_os_name(...)` stability `stable`
- `proc get_os_version` signature `proc get_os_version() -> string {` example `sysinfo.get_os_version(...)` stability `stable`
- `proc get_architecture` signature `proc get_architecture() -> string {` example `sysinfo.get_architecture(...)` stability `stable`
- `proc is_windows` signature `proc is_windows() -> int {` example `sysinfo.is_windows(...)` stability `stable`
- `proc is_linux` signature `proc is_linux() -> int {` example `sysinfo.is_linux(...)` stability `stable`
- `proc is_macos` signature `proc is_macos() -> int {` example `sysinfo.is_macos(...)` stability `stable`
- `proc is_unix` signature `proc is_unix() -> int {` example `sysinfo.is_unix(...)` stability `stable`
- `proc get_cpu_info` signature `proc get_cpu_info() -> CPUInfo {` example `sysinfo.get_cpu_info(...)` stability `stable`
- `proc get_cpu_count` signature `proc get_cpu_count() -> i32 {` example `sysinfo.get_cpu_count(...)` stability `stable`
- `proc get_cpu_count_physical` signature `proc get_cpu_count_physical() -> i32 {` example `sysinfo.get_cpu_count_physical(...)` stability `stable`
- `proc get_cpu_frequency` signature `proc get_cpu_frequency() -> f64 {` example `sysinfo.get_cpu_frequency(...)` stability `stable`
- `proc get_cpu_usage` signature `proc get_cpu_usage() -> f64 {` example `sysinfo.get_cpu_usage(...)` stability `stable`
- `proc get_process_cpu_usage` signature `proc get_process_cpu_usage() -> f64 {` example `sysinfo.get_process_cpu_usage(...)` stability `stable`
- `proc get_memory_info` signature `proc get_memory_info() -> MemoryInfo {` example `sysinfo.get_memory_info(...)` stability `stable`
- `proc get_total_memory` signature `proc get_total_memory() -> i64 {` example `sysinfo.get_total_memory(...)` stability `stable`
- `proc get_available_memory` signature `proc get_available_memory() -> i64 {` example `sysinfo.get_available_memory(...)` stability `stable`
- `proc get_used_memory` signature `proc get_used_memory() -> i64 {` example `sysinfo.get_used_memory(...)` stability `stable`
- `proc get_free_memory` signature `proc get_free_memory() -> i64 {` example `sysinfo.get_free_memory(...)` stability `stable`
- `proc get_memory_percent` signature `proc get_memory_percent() -> f64 {` example `sysinfo.get_memory_percent(...)` stability `stable`
- `proc get_process_memory_mb` signature `proc get_process_memory_mb() -> i32 {` example `sysinfo.get_process_memory_mb(...)` stability `stable`
- `proc get_disk_usage` signature `proc get_disk_usage(path: string) -> DiskStat {` example `sysinfo.get_disk_usage(...)` stability `stable`
- `proc get_disk_usage_all` signature `proc get_disk_usage_all() -> [DiskStat] {` example `sysinfo.get_disk_usage_all(...)` stability `stable`
- `proc get_total_disk_space` signature `proc get_total_disk_space(path: string) -> i64 {` example `sysinfo.get_total_disk_space(...)` stability `stable`
- `proc get_used_disk_space` signature `proc get_used_disk_space(path: string) -> i64 {` example `sysinfo.get_used_disk_space(...)` stability `stable`
- `proc get_free_disk_space` signature `proc get_free_disk_space(path: string) -> i64 {` example `sysinfo.get_free_disk_space(...)` stability `stable`
- `proc get_process_info` signature `proc get_process_info(pid: i32) -> ProcessInfo {` example `sysinfo.get_process_info(...)` stability `stable`
- `proc get_current_process_id` signature `proc get_current_process_id() -> i32 {` example `sysinfo.get_current_process_id(...)` stability `stable`
- `proc get_parent_process_id` signature `proc get_parent_process_id() -> i32 {` example `sysinfo.get_parent_process_id(...)` stability `stable`
- `proc get_running_processes` signature `proc get_running_processes() -> [ProcessInfo] {` example `sysinfo.get_running_processes(...)` stability `stable`
- `proc get_process_count` signature `proc get_process_count() -> i32 {` example `sysinfo.get_process_count(...)` stability `stable`
- `proc get_thread_count` signature `proc get_thread_count() -> i32 {` example `sysinfo.get_thread_count(...)` stability `stable`
- `proc get_network_interfaces` signature `proc get_network_interfaces() -> [NetworkStat] {` example `sysinfo.get_network_interfaces(...)` stability `stable`
- `proc get_network_interface` signature `proc get_network_interface(name: string) -> NetworkStat {` example `sysinfo.get_network_interface(...)` stability `stable`
- `proc get_hostname` signature `proc get_hostname() -> string {` example `sysinfo.get_hostname(...)` stability `stable`
- `proc get_local_ip` signature `proc get_local_ip() -> string {` example `sysinfo.get_local_ip(...)` stability `stable`
- `proc get_mac_address` signature `proc get_mac_address(interface: string) -> string {` example `sysinfo.get_mac_address(...)` stability `stable`
- `proc get_uptime_seconds` signature `proc get_uptime_seconds() -> i64 {` example `sysinfo.get_uptime_seconds(...)` stability `stable`
- `proc get_boot_time` signature `proc get_boot_time() -> i64 {` example `sysinfo.get_boot_time(...)` stability `stable`
- `proc get_system_load_average` signature `proc get_system_load_average() -> [f64] {` example `sysinfo.get_system_load_average(...)` stability `stable`
- `proc get_battery_percent` signature `proc get_battery_percent() -> i32 {` example `sysinfo.get_battery_percent(...)` stability `stable`
- `proc get_battery_time_remaining` signature `proc get_battery_time_remaining() -> i32 {` example `sysinfo.get_battery_time_remaining(...)` stability `stable`
- `proc is_on_battery` signature `proc is_on_battery() -> int {` example `sysinfo.is_on_battery(...)` stability `stable`
- `proc is_charging` signature `proc is_charging() -> int {` example `sysinfo.is_charging(...)` stability `stable`
- `proc get_battery_info` signature `proc get_battery_info() -> BatteryInfo {` example `sysinfo.get_battery_info(...)` stability `stable`
- `proc get_system_health_report` signature `proc get_system_health_report() -> SystemHealthReport {` example `sysinfo.get_system_health_report(...)` stability `stable`
- `proc get_current_process_info` signature `proc get_current_process_info() -> ProcessInfo {` example `sysinfo.get_current_process_info(...)` stability `stable`
- `proc get_system_load` signature `proc get_system_load() -> SystemLoad {` example `sysinfo.get_system_load(...)` stability `stable`
- `proc get_primary_disk_usage` signature `proc get_primary_disk_usage() -> DiskStat {` example `sysinfo.get_primary_disk_usage(...)` stability `stable`
- `proc get_primary_network_stat` signature `proc get_primary_network_stat() -> NetworkStat {` example `sysinfo.get_primary_network_stat(...)` stability `stable`
- `proc get_system_snapshot` signature `proc get_system_snapshot() -> SystemSnapshot {` example `sysinfo.get_system_snapshot(...)` stability `stable`
- `proc get_system_summary` signature `proc get_system_summary() -> SystemSummary {` example `sysinfo.get_system_summary(...)` stability `stable`
- `proc get_sysinfo_report` signature `proc get_sysinfo_report() -> SysInfoReport {` example `sysinfo.get_sysinfo_report(...)` stability `stable`
- `proc sysinfo_health` signature `proc sysinfo_health() -> bool {` example `sysinfo.sysinfo_health(...)` stability `stable`
- `proc sysinfo_selftest` signature `proc sysinfo_selftest() -> bool {` example `sysinfo.sysinfo_selftest(...)` stability `stable`

## `src/vitte/stdlib/datetime.vitl`

Stability: `stable`

- `form DatetimeLibraryManifest` signature `form DatetimeLibraryManifest {` example `datetime.DatetimeLibraryManifest` stability `stable`
- `form DatetimeLibraryHealth` signature `form DatetimeLibraryHealth {` example `datetime.DatetimeLibraryHealth` stability `stable`
- `form DatetimeLibrarySummary` signature `form DatetimeLibrarySummary {` example `datetime.DatetimeLibrarySummary` stability `stable`
- `form DatetimeLibraryReport` signature `form DatetimeLibraryReport {` example `datetime.DatetimeLibraryReport` stability `stable`
- `const NANOS_PER_MICRO` signature `const NANOS_PER_MICRO: i64 = 0` example `datetime.NANOS_PER_MICRO` stability `stable`
- `const NANOS_PER_MILLI` signature `const NANOS_PER_MILLI: i64 = 0` example `datetime.NANOS_PER_MILLI` stability `stable`
- `const NANOS_PER_SECOND` signature `const NANOS_PER_SECOND: i64 = 0` example `datetime.NANOS_PER_SECOND` stability `stable`
- `const SECONDS_PER_MINUTE` signature `const SECONDS_PER_MINUTE: i64 = 0` example `datetime.SECONDS_PER_MINUTE` stability `stable`
- `const MINUTES_PER_HOUR` signature `const MINUTES_PER_HOUR: i64 = 0` example `datetime.MINUTES_PER_HOUR` stability `stable`
- `const HOURS_PER_DAY` signature `const HOURS_PER_DAY: i64 = 0` example `datetime.HOURS_PER_DAY` stability `stable`
- `const SECONDS_PER_HOUR` signature `const SECONDS_PER_HOUR: i64 = 0` example `datetime.SECONDS_PER_HOUR` stability `stable`
- `const SECONDS_PER_DAY` signature `const SECONDS_PER_DAY: i64 = 0` example `datetime.SECONDS_PER_DAY` stability `stable`
- `const MILLIS_PER_SECOND` signature `const MILLIS_PER_SECOND: i64 = 0` example `datetime.MILLIS_PER_SECOND` stability `stable`
- `const MICROS_PER_SECOND` signature `const MICROS_PER_SECOND: i64 = 0` example `datetime.MICROS_PER_SECOND` stability `stable`
- `const UNIX_EPOCH_YEAR` signature `const UNIX_EPOCH_YEAR: i32 = 0` example `datetime.UNIX_EPOCH_YEAR` stability `stable`
- `proc datetime_library_version` signature `proc datetime_library_version() -> int {` example `datetime.datetime_library_version(...)` stability `stable`
- `proc datetime_library_name` signature `proc datetime_library_name() -> int {` example `datetime.datetime_library_name(...)` stability `stable`
- `proc datetime_library_module_count` signature `proc datetime_library_module_count() -> int {` example `datetime.datetime_library_module_count(...)` stability `stable`
- `proc datetime_library_modules` signature `proc datetime_library_modules() -> int {` example `datetime.datetime_library_modules(...)` stability `stable`
- `proc datetime_library_manifest` signature `proc datetime_library_manifest() -> int {` example `datetime.datetime_library_manifest(...)` stability `stable`
- `proc datetime_library_ready` signature `proc datetime_library_ready() -> int {` example `datetime.datetime_library_ready(...)` stability `stable`
- `proc datetime_library_health` signature `proc datetime_library_health() -> int {` example `datetime.datetime_library_health(...)` stability `stable`
- `proc datetime_library_summary` signature `proc datetime_library_summary() -> int {` example `datetime.datetime_library_summary(...)` stability `stable`
- `proc datetime_library_selftest` signature `proc datetime_library_selftest() -> int {` example `datetime.datetime_library_selftest(...)` stability `stable`
- `proc datetime_library_report` signature `proc datetime_library_report() -> int {` example `datetime.datetime_library_report(...)` stability `stable`
- `form Duration` signature `form Duration {` example `datetime.Duration` stability `stable`
- `form Date` signature `form Date {` example `datetime.Date` stability `stable`
- `form Time` signature `form Time {` example `datetime.Time` stability `stable`
- `form DateTime` signature `form DateTime {` example `datetime.DateTime` stability `stable`
- `form UnixTimestamp` signature `form UnixTimestamp {` example `datetime.UnixTimestamp` stability `stable`
- `pick Weekday` signature `pick Weekday {` example `datetime.Weekday` stability `stable`
- `pick Month` signature `pick Month {` example `datetime.Month` stability `stable`
- `pick MaybeMonth` signature `pick MaybeMonth {` example `datetime.MaybeMonth` stability `stable`
- `pick DateTimeError` signature `pick DateTimeError {` example `datetime.DateTimeError` stability `stable`
- `pick MaybeDate` signature `pick MaybeDate {` example `datetime.MaybeDate` stability `stable`
- `pick MaybeTime` signature `pick MaybeTime {` example `datetime.MaybeTime` stability `stable`
- `pick MaybeDateTime` signature `pick MaybeDateTime {` example `datetime.MaybeDateTime` stability `stable`
- `proc duration_new` signature `proc duration_new() -> int {` example `datetime.duration_new(...)` stability `stable`
- `proc duration_zero` signature `proc duration_zero() -> int {` example `datetime.duration_zero(...)` stability `stable`
- `proc duration_from_seconds` signature `proc duration_from_seconds() -> int {` example `datetime.duration_from_seconds(...)` stability `stable`
- `proc duration_from_millis` signature `proc duration_from_millis() -> int {` example `datetime.duration_from_millis(...)` stability `stable`
- `proc duration_from_micros` signature `proc duration_from_micros() -> int {` example `datetime.duration_from_micros(...)` stability `stable`
- `proc duration_from_nanos` signature `proc duration_from_nanos() -> int {` example `datetime.duration_from_nanos(...)` stability `stable`
- `proc duration_total_seconds` signature `proc duration_total_seconds() -> int {` example `datetime.duration_total_seconds(...)` stability `stable`
- `proc duration_total_millis` signature `proc duration_total_millis() -> int {` example `datetime.duration_total_millis(...)` stability `stable`
- `proc duration_total_micros` signature `proc duration_total_micros() -> int {` example `datetime.duration_total_micros(...)` stability `stable`
- `proc duration_total_nanos` signature `proc duration_total_nanos() -> int {` example `datetime.duration_total_nanos(...)` stability `stable`
- `proc duration_add` signature `proc duration_add() -> int {` example `datetime.duration_add(...)` stability `stable`
- `proc duration_sub` signature `proc duration_sub() -> int {` example `datetime.duration_sub(...)` stability `stable`
- `proc duration_neg` signature `proc duration_neg() -> int {` example `datetime.duration_neg(...)` stability `stable`
- `proc duration_cmp` signature `proc duration_cmp() -> int {` example `datetime.duration_cmp(...)` stability `stable`
- `proc duration_eq` signature `proc duration_eq() -> int {` example `datetime.duration_eq(...)` stability `stable`
- `proc duration_lt` signature `proc duration_lt() -> int {` example `datetime.duration_lt(...)` stability `stable`
- `proc duration_lte` signature `proc duration_lte() -> int {` example `datetime.duration_lte(...)` stability `stable`
- `proc duration_gt` signature `proc duration_gt() -> int {` example `datetime.duration_gt(...)` stability `stable`
- `proc duration_gte` signature `proc duration_gte() -> int {` example `datetime.duration_gte(...)` stability `stable`
- `proc is_leap_year` signature `proc is_leap_year() -> int {` example `datetime.is_leap_year(...)` stability `stable`
- `proc days_in_month` signature `proc days_in_month() -> int {` example `datetime.days_in_month(...)` stability `stable`
- `proc days_in_year` signature `proc days_in_year() -> int {` example `datetime.days_in_year(...)` stability `stable`
- `proc month_to_number` signature `proc month_to_number() -> int {` example `datetime.month_to_number(...)` stability `stable`
- `proc number_to_month` signature `proc number_to_month() -> int {` example `datetime.number_to_month(...)` stability `stable`
- `proc month_name` signature `proc month_name() -> int {` example `datetime.month_name(...)` stability `stable`
- `proc month_short_name` signature `proc month_short_name() -> int {` example `datetime.month_short_name(...)` stability `stable`
- `proc weekday_name` signature `proc weekday_name() -> int {` example `datetime.weekday_name(...)` stability `stable`
- `proc weekday_short_name` signature `proc weekday_short_name() -> int {` example `datetime.weekday_short_name(...)` stability `stable`
- `proc validate_date` signature `proc validate_date() -> int {` example `datetime.validate_date(...)` stability `stable`
- `proc validate_time` signature `proc validate_time() -> int {` example `datetime.validate_time(...)` stability `stable`
- `proc validate_datetime` signature `proc validate_datetime() -> int {` example `datetime.validate_datetime(...)` stability `stable`
- `proc date_new` signature `proc date_new() -> int {` example `datetime.date_new(...)` stability `stable`
- `proc time_new` signature `proc time_new() -> int {` example `datetime.time_new(...)` stability `stable`
- `proc datetime_new` signature `proc datetime_new() -> int {` example `datetime.datetime_new(...)` stability `stable`
- `proc date_ordinal` signature `proc date_ordinal() -> int {` example `datetime.date_ordinal(...)` stability `stable`
- `proc date_from_ordinal` signature `proc date_from_ordinal() -> int {` example `datetime.date_from_ordinal(...)` stability `stable`
- `proc days_before_year` signature `proc days_before_year() -> int {` example `datetime.days_before_year(...)` stability `stable`
- `proc days_before_month` signature `proc days_before_month() -> int {` example `datetime.days_before_month(...)` stability `stable`
- `proc date_to_days_since_epoch` signature `proc date_to_days_since_epoch() -> int {` example `datetime.date_to_days_since_epoch(...)` stability `stable`
- `proc days_since_epoch_to_date` signature `proc days_since_epoch_to_date() -> int {` example `datetime.days_since_epoch_to_date(...)` stability `stable`
- `proc time_to_seconds` signature `proc time_to_seconds() -> int {` example `datetime.time_to_seconds(...)` stability `stable`
- `proc seconds_to_time` signature `proc seconds_to_time() -> int {` example `datetime.seconds_to_time(...)` stability `stable`
- `proc datetime_to_unix` signature `proc datetime_to_unix() -> int {` example `datetime.datetime_to_unix(...)` stability `stable`
- `proc unix_to_datetime` signature `proc unix_to_datetime() -> int {` example `datetime.unix_to_datetime(...)` stability `stable`
- `proc unix_normalize` signature `proc unix_normalize() -> int {` example `datetime.unix_normalize(...)` stability `stable`
- `proc unix_from_seconds` signature `proc unix_from_seconds() -> int {` example `datetime.unix_from_seconds(...)` stability `stable`
- `proc unix_from_millis` signature `proc unix_from_millis() -> int {` example `datetime.unix_from_millis(...)` stability `stable`
- `proc unix_to_millis` signature `proc unix_to_millis() -> int {` example `datetime.unix_to_millis(...)` stability `stable`
- `proc floor_div` signature `proc floor_div() -> int {` example `datetime.floor_div(...)` stability `stable`
- `proc floor_mod` signature `proc floor_mod() -> int {` example `datetime.floor_mod(...)` stability `stable`
- `proc weekday_from_days` signature `proc weekday_from_days() -> int {` example `datetime.weekday_from_days(...)` stability `stable`
- `proc date_weekday` signature `proc date_weekday() -> int {` example `datetime.date_weekday(...)` stability `stable`
- `proc date_add_days` signature `proc date_add_days() -> int {` example `datetime.date_add_days(...)` stability `stable`
- `proc datetime_add_duration` signature `proc datetime_add_duration() -> int {` example `datetime.datetime_add_duration(...)` stability `stable`
- `proc datetime_sub_duration` signature `proc datetime_sub_duration() -> int {` example `datetime.datetime_sub_duration(...)` stability `stable`
- `proc datetime_duration_between` signature `proc datetime_duration_between() -> int {` example `datetime.datetime_duration_between(...)` stability `stable`
- `proc date_cmp` signature `proc date_cmp() -> int {` example `datetime.date_cmp(...)` stability `stable`
- `proc time_cmp` signature `proc time_cmp() -> int {` example `datetime.time_cmp(...)` stability `stable`
- `proc datetime_cmp` signature `proc datetime_cmp() -> int {` example `datetime.datetime_cmp(...)` stability `stable`
- `proc date_eq` signature `proc date_eq() -> int {` example `datetime.date_eq(...)` stability `stable`
- `proc time_eq` signature `proc time_eq() -> int {` example `datetime.time_eq(...)` stability `stable`
- `proc datetime_eq` signature `proc datetime_eq() -> int {` example `datetime.datetime_eq(...)` stability `stable`
- `proc date_is_before` signature `proc date_is_before() -> int {` example `datetime.date_is_before(...)` stability `stable`
- `proc date_is_after` signature `proc date_is_after() -> int {` example `datetime.date_is_after(...)` stability `stable`
- `proc datetime_is_before` signature `proc datetime_is_before() -> int {` example `datetime.datetime_is_before(...)` stability `stable`
- `proc datetime_is_after` signature `proc datetime_is_after() -> int {` example `datetime.datetime_is_after(...)` stability `stable`
- `proc clamp_u8` signature `proc clamp_u8() -> int {` example `datetime.clamp_u8(...)` stability `stable`
- `proc ascii_digit_value` signature `proc ascii_digit_value() -> int {` example `datetime.ascii_digit_value(...)` stability `stable`
- `proc is_ascii_digit` signature `proc is_ascii_digit() -> int {` example `datetime.is_ascii_digit(...)` stability `stable`
- `proc parse_2_digits` signature `proc parse_2_digits() -> int {` example `datetime.parse_2_digits(...)` stability `stable`
- `proc parse_4_digits` signature `proc parse_4_digits() -> int {` example `datetime.parse_4_digits(...)` stability `stable`
- `proc parse_date_yyyy_mm_dd` signature `proc parse_date_yyyy_mm_dd() -> int {` example `datetime.parse_date_yyyy_mm_dd(...)` stability `stable`
- `proc parse_time_hh_mm_ss` signature `proc parse_time_hh_mm_ss() -> int {` example `datetime.parse_time_hh_mm_ss(...)` stability `stable`
- `proc parse_datetime_iso_basic` signature `proc parse_datetime_iso_basic() -> int {` example `datetime.parse_datetime_iso_basic(...)` stability `stable`
- `proc two_digits` signature `proc two_digits() -> int {` example `datetime.two_digits(...)` stability `stable`
- `proc four_digits` signature `proc four_digits() -> int {` example `datetime.four_digits(...)` stability `stable`
- `proc format_date_iso` signature `proc format_date_iso() -> int {` example `datetime.format_date_iso(...)` stability `stable`
- `proc format_time_iso` signature `proc format_time_iso() -> int {` example `datetime.format_time_iso(...)` stability `stable`
- `proc format_datetime_iso` signature `proc format_datetime_iso() -> int {` example `datetime.format_datetime_iso(...)` stability `stable`
- `proc format_datetime_space` signature `proc format_datetime_space() -> int {` example `datetime.format_datetime_space(...)` stability `stable`
- `proc format_date_human` signature `proc format_date_human() -> int {` example `datetime.format_date_human(...)` stability `stable`
- `proc format_weekday_date` signature `proc format_weekday_date() -> int {` example `datetime.format_weekday_date(...)` stability `stable`
- `proc start_of_day` signature `proc start_of_day() -> int {` example `datetime.start_of_day(...)` stability `stable`
- `proc end_of_day` signature `proc end_of_day() -> int {` example `datetime.end_of_day(...)` stability `stable`
- `proc date_next_day` signature `proc date_next_day() -> int {` example `datetime.date_next_day(...)` stability `stable`
- `proc date_prev_day` signature `proc date_prev_day() -> int {` example `datetime.date_prev_day(...)` stability `stable`
- `proc date_add_weeks` signature `proc date_add_weeks() -> int {` example `datetime.date_add_weeks(...)` stability `stable`
- `proc date_start_of_month` signature `proc date_start_of_month() -> int {` example `datetime.date_start_of_month(...)` stability `stable`
- `proc date_end_of_month` signature `proc date_end_of_month() -> int {` example `datetime.date_end_of_month(...)` stability `stable`
- `proc date_start_of_year` signature `proc date_start_of_year() -> int {` example `datetime.date_start_of_year(...)` stability `stable`
- `proc date_end_of_year` signature `proc date_end_of_year() -> int {` example `datetime.date_end_of_year(...)` stability `stable`
- `proc date_add_months` signature `proc date_add_months() -> int {` example `datetime.date_add_months(...)` stability `stable`
- `proc date_add_years` signature `proc date_add_years() -> int {` example `datetime.date_add_years(...)` stability `stable`
- `proc floor_div_i32` signature `proc floor_div_i32() -> int {` example `datetime.floor_div_i32(...)` stability `stable`
- `proc floor_mod_i32` signature `proc floor_mod_i32() -> int {` example `datetime.floor_mod_i32(...)` stability `stable`
- `proc iso_weekday_number` signature `proc iso_weekday_number() -> int {` example `datetime.iso_weekday_number(...)` stability `stable`
- `proc date_iso_weekday_number` signature `proc date_iso_weekday_number() -> int {` example `datetime.date_iso_weekday_number(...)` stability `stable`
- `proc is_weekend` signature `proc is_weekend() -> int {` example `datetime.is_weekend(...)` stability `stable`
- `proc is_weekday` signature `proc is_weekday() -> int {` example `datetime.is_weekday(...)` stability `stable`
- `proc date_days_between` signature `proc date_days_between() -> int {` example `datetime.date_days_between(...)` stability `stable`
- `proc datetime_seconds_between` signature `proc datetime_seconds_between() -> int {` example `datetime.datetime_seconds_between(...)` stability `stable`
- `proc unix_epoch_datetime` signature `proc unix_epoch_datetime() -> int {` example `datetime.unix_epoch_datetime(...)` stability `stable`
- `proc unix_epoch_date` signature `proc unix_epoch_date() -> int {` example `datetime.unix_epoch_date(...)` stability `stable`
- `proc midnight` signature `proc midnight() -> int {` example `datetime.midnight(...)` stability `stable`
- `proc noon` signature `proc noon() -> int {` example `datetime.noon(...)` stability `stable`
- `proc time_add_duration` signature `proc time_add_duration() -> int {` example `datetime.time_add_duration(...)` stability `stable`
- `proc time_is_midnight` signature `proc time_is_midnight() -> int {` example `datetime.time_is_midnight(...)` stability `stable`
- `proc time_is_noon` signature `proc time_is_noon() -> int {` example `datetime.time_is_noon(...)` stability `stable`
- `proc date_quarter` signature `proc date_quarter() -> int {` example `datetime.date_quarter(...)` stability `stable`
- `proc date_day_of_year` signature `proc date_day_of_year() -> int {` example `datetime.date_day_of_year(...)` stability `stable`
- `proc date_days_remaining_in_year` signature `proc date_days_remaining_in_year() -> int {` example `datetime.date_days_remaining_in_year(...)` stability `stable`
- `proc date_days_remaining_in_month` signature `proc date_days_remaining_in_month() -> int {` example `datetime.date_days_remaining_in_month(...)` stability `stable`
- `proc date_is_first_day_of_month` signature `proc date_is_first_day_of_month() -> int {` example `datetime.date_is_first_day_of_month(...)` stability `stable`
- `proc date_is_last_day_of_month` signature `proc date_is_last_day_of_month() -> int {` example `datetime.date_is_last_day_of_month(...)` stability `stable`
- `proc date_is_first_day_of_year` signature `proc date_is_first_day_of_year() -> int {` example `datetime.date_is_first_day_of_year(...)` stability `stable`
- `proc date_is_last_day_of_year` signature `proc date_is_last_day_of_year() -> int {` example `datetime.date_is_last_day_of_year(...)` stability `stable`
- `proc timestamp_cmp` signature `proc timestamp_cmp() -> int {` example `datetime.timestamp_cmp(...)` stability `stable`
- `proc timestamp_add_duration` signature `proc timestamp_add_duration() -> int {` example `datetime.timestamp_add_duration(...)` stability `stable`
- `proc timestamp_sub_duration` signature `proc timestamp_sub_duration() -> int {` example `datetime.timestamp_sub_duration(...)` stability `stable`
- `proc timestamp_duration_between` signature `proc timestamp_duration_between() -> int {` example `datetime.timestamp_duration_between(...)` stability `stable`
- `proc datetime_now_unavailable` signature `proc datetime_now_unavailable() -> int {` example `datetime.datetime_now_unavailable(...)` stability `stable`
- `proc string_len` signature `proc string_len() -> int {` example `datetime.string_len(...)` stability `stable`
- `proc string_at` signature `proc string_at() -> int {` example `datetime.string_at(...)` stability `stable`
- `proc string_slice` signature `proc string_slice() -> int {` example `datetime.string_slice(...)` stability `stable`
- `proc string_concat` signature `proc string_concat() -> int {` example `datetime.string_concat(...)` stability `stable`
- `proc string_concat_many` signature `proc string_concat_many() -> int {` example `datetime.string_concat_many(...)` stability `stable`
- `proc int_to_string` signature `proc int_to_string() -> int {` example `datetime.int_to_string(...)` stability `stable`
- `proc datetime_selftest` signature `proc datetime_selftest() -> int {` example `datetime.datetime_selftest(...)` stability `stable`

## `src/vitte/stdlib/runtime.vitl`

Stability: `stable`

- `form String` signature `form String {` example `runtime.String` stability `stable`
- `form Slice` signature `form Slice {` example `runtime.Slice` stability `stable`
- `form Pair` signature `form Pair {` example `runtime.Pair` stability `stable`
- `pick OptionString` signature `pick OptionString {` example `runtime.OptionString` stability `stable`
- `pick IoErrorKind` signature `pick IoErrorKind {` example `runtime.IoErrorKind` stability `stable`
- `form IoError` signature `form IoError {` example `runtime.IoError` stability `stable`
- `pick ResultIo` signature `pick ResultIo {` example `runtime.ResultIo` stability `stable`
- `pick Result` signature `pick Result {` example `runtime.Result` stability `stable`
- `form IpV4` signature `form IpV4 {` example `runtime.IpV4` stability `stable`
- `form IpV6` signature `form IpV6 {` example `runtime.IpV6` stability `stable`
- `pick IpAddr` signature `pick IpAddr {` example `runtime.IpAddr` stability `stable`
- `form SocketAddr` signature `form SocketAddr {` example `runtime.SocketAddr` stability `stable`
- `form TcpStream` signature `form TcpStream {` example `runtime.TcpStream` stability `stable`
- `form TcpListener` signature `form TcpListener {` example `runtime.TcpListener` stability `stable`
- `form UdpSocket` signature `form UdpSocket {` example `runtime.UdpSocket` stability `stable`
- `form UdpRecv` signature `form UdpRecv {` example `runtime.UdpRecv` stability `stable`
- `pick JsonValue` signature `pick JsonValue {` example `runtime.JsonValue` stability `stable`
- `form JsonMember` signature `form JsonMember {` example `runtime.JsonMember` stability `stable`
- `pick RegexMatch` signature `pick RegexMatch {` example `runtime.RegexMatch` stability `stable`
- `pick OptionRegexMatch` signature `pick OptionRegexMatch {` example `runtime.OptionRegexMatch` stability `stable`
- `form Regex` signature `form Regex {` example `runtime.Regex` stability `stable`
- `form ProcessResult` signature `form ProcessResult {` example `runtime.ProcessResult` stability `stable`
- `form ExitStatus` signature `form ExitStatus {` example `runtime.ExitStatus` stability `stable`
- `form ProcessChild` signature `form ProcessChild {` example `runtime.ProcessChild` stability `stable`
- `form FswatchWatcher` signature `form FswatchWatcher {` example `runtime.FswatchWatcher` stability `stable`
- `pick FswatchEventKind` signature `pick FswatchEventKind {` example `runtime.FswatchEventKind` stability `stable`
- `form FswatchEvent` signature `form FswatchEvent {` example `runtime.FswatchEvent` stability `stable`
- `form DbHandle` signature `form DbHandle {` example `runtime.DbHandle` stability `stable`
- `form DbEntry` signature `form DbEntry {` example `runtime.DbEntry` stability `stable`
- `pick HttpMethod` signature `pick HttpMethod {` example `runtime.HttpMethod` stability `stable`
- `form HttpHeader` signature `form HttpHeader {` example `runtime.HttpHeader` stability `stable`
- `form HttpRequest` signature `form HttpRequest {` example `runtime.HttpRequest` stability `stable`
- `form HttpResponse` signature `form HttpResponse {` example `runtime.HttpResponse` stability `stable`
- `form Unit` signature `form Unit {` example `runtime.Unit` stability `stable`
- `proc alloc` signature `proc alloc() -> int {` example `runtime.alloc(...)` stability `stable`
- `proc dealloc` signature `proc dealloc() -> int {` example `runtime.dealloc(...)` stability `stable`
- `proc runtime_panic` signature `proc runtime_panic() -> int {` example `runtime.runtime_panic(...)` stability `stable`
- `proc runtime_panic_boundary_begin` signature `proc runtime_panic_boundary_begin() -> int {` example `runtime.runtime_panic_boundary_begin(...)` stability `stable`
- `proc runtime_panic_boundary_end` signature `proc runtime_panic_boundary_end() -> int {` example `runtime.runtime_panic_boundary_end(...)` stability `stable`
- `proc runtime_panic_boundary_triggered` signature `proc runtime_panic_boundary_triggered() -> int {` example `runtime.runtime_panic_boundary_triggered(...)` stability `stable`
- `proc runtime_panic_boundary_code` signature `proc runtime_panic_boundary_code() -> int {` example `runtime.runtime_panic_boundary_code(...)` stability `stable`
- `proc runtime_panic_boundary_reset` signature `proc runtime_panic_boundary_reset() -> int {` example `runtime.runtime_panic_boundary_reset(...)` stability `stable`
- `proc assert_true` signature `proc assert_true() -> int {` example `runtime.assert_true(...)` stability `stable`
- `proc print_i32` signature `proc print_i32() -> int {` example `runtime.print_i32(...)` stability `stable`
- `proc runtime_unreachable` signature `proc runtime_unreachable() -> int {` example `runtime.runtime_unreachable(...)` stability `stable`
- `proc terminate` signature `proc terminate() -> int {` example `runtime.terminate(...)` stability `stable`
- `proc vitte_c_abi_version` signature `proc vitte_c_abi_version() -> int {` example `runtime.vitte_c_abi_version(...)` stability `stable`
- `proc empty_string` signature `proc empty_string() -> int {` example `runtime.empty_string(...)` stability `stable`
- `proc make_string` signature `proc make_string() -> int {` example `runtime.make_string(...)` stability `stable`
- `proc char_to_string` signature `proc char_to_string() -> int {` example `runtime.char_to_string(...)` stability `stable`
- `proc i32_to_string` signature `proc i32_to_string() -> int {` example `runtime.i32_to_string(...)` stability `stable`
- `proc string_concat` signature `proc string_concat() -> int {` example `runtime.string_concat(...)` stability `stable`
- `proc empty_slice_i32` signature `proc empty_slice_i32() -> int {` example `runtime.empty_slice_i32(...)` stability `stable`
- `proc empty_slice_string` signature `proc empty_slice_string() -> int {` example `runtime.empty_slice_string(...)` stability `stable`
- `proc slice_push_i32` signature `proc slice_push_i32() -> int {` example `runtime.slice_push_i32(...)` stability `stable`
- `proc slice_push_string` signature `proc slice_push_string() -> int {` example `runtime.slice_push_string(...)` stability `stable`
- `proc list_i32` signature `proc list_i32() -> int {` example `runtime.list_i32(...)` stability `stable`
- `proc list_string` signature `proc list_string() -> int {` example `runtime.list_string(...)` stability `stable`
- `proc time_now_ms` signature `proc time_now_ms() -> int {` example `runtime.time_now_ms(...)` stability `stable`
- `proc time_sleep_ms` signature `proc time_sleep_ms() -> int {` example `runtime.time_sleep_ms(...)` stability `stable`
- `proc env_get` signature `proc env_get() -> int {` example `runtime.env_get(...)` stability `stable`
- `proc env_set` signature `proc env_set() -> int {` example `runtime.env_set(...)` stability `stable`
- `proc os_platform` signature `proc os_platform() -> int {` example `runtime.os_platform(...)` stability `stable`
- `proc os_arch` signature `proc os_arch() -> int {` example `runtime.os_arch(...)` stability `stable`
- `proc os_home_dir` signature `proc os_home_dir() -> int {` example `runtime.os_home_dir(...)` stability `stable`
- `proc os_temp_dir` signature `proc os_temp_dir() -> int {` example `runtime.os_temp_dir(...)` stability `stable`
- `proc os_current_dir` signature `proc os_current_dir() -> int {` example `runtime.os_current_dir(...)` stability `stable`
- `proc os_set_current_dir` signature `proc os_set_current_dir() -> int {` example `runtime.os_set_current_dir(...)` stability `stable`
- `proc os_exe_path` signature `proc os_exe_path() -> int {` example `runtime.os_exe_path(...)` stability `stable`
- `proc os_path_sep` signature `proc os_path_sep() -> int {` example `runtime.os_path_sep(...)` stability `stable`
- `proc process_run` signature `proc process_run() -> int {` example `runtime.process_run(...)` stability `stable`
- `proc process_run_args` signature `proc process_run_args() -> int {` example `runtime.process_run_args(...)` stability `stable`
- `proc process_run_shell` signature `proc process_run_shell() -> int {` example `runtime.process_run_shell(...)` stability `stable`
- `proc process_spawn` signature `proc process_spawn() -> int {` example `runtime.process_spawn(...)` stability `stable`
- `proc process_wait` signature `proc process_wait() -> int {` example `runtime.process_wait(...)` stability `stable`
- `proc process_kill` signature `proc process_kill() -> int {` example `runtime.process_kill(...)` stability `stable`
- `proc process_stdout` signature `proc process_stdout() -> int {` example `runtime.process_stdout(...)` stability `stable`
- `proc process_stderr` signature `proc process_stderr() -> int {` example `runtime.process_stderr(...)` stability `stable`
- `proc json_parse` signature `proc json_parse() -> int {` example `runtime.json_parse(...)` stability `stable`
- `proc json_stringify` signature `proc json_stringify() -> int {` example `runtime.json_stringify(...)` stability `stable`
- `proc http_request` signature `proc http_request() -> int {` example `runtime.http_request(...)` stability `stable`
- `proc crypto_sha256` signature `proc crypto_sha256() -> int {` example `runtime.crypto_sha256(...)` stability `stable`
- `proc crypto_sha1` signature `proc crypto_sha1() -> int {` example `runtime.crypto_sha1(...)` stability `stable`
- `proc crypto_hmac_sha256` signature `proc crypto_hmac_sha256() -> int {` example `runtime.crypto_hmac_sha256(...)` stability `stable`
- `proc crypto_rand_bytes` signature `proc crypto_rand_bytes() -> int {` example `runtime.crypto_rand_bytes(...)` stability `stable`
- `proc tcp_connect` signature `proc tcp_connect() -> int {` example `runtime.tcp_connect(...)` stability `stable`
- `proc tcp_bind` signature `proc tcp_bind() -> int {` example `runtime.tcp_bind(...)` stability `stable`
- `proc tcp_accept` signature `proc tcp_accept() -> int {` example `runtime.tcp_accept(...)` stability `stable`
- `proc tcp_read` signature `proc tcp_read() -> int {` example `runtime.tcp_read(...)` stability `stable`
- `proc tcp_write` signature `proc tcp_write() -> int {` example `runtime.tcp_write(...)` stability `stable`
- `proc tcp_close` signature `proc tcp_close() -> int {` example `runtime.tcp_close(...)` stability `stable`
- `proc udp_bind` signature `proc udp_bind() -> int {` example `runtime.udp_bind(...)` stability `stable`
- `proc udp_recv_from` signature `proc udp_recv_from() -> int {` example `runtime.udp_recv_from(...)` stability `stable`
- `proc udp_send_to` signature `proc udp_send_to() -> int {` example `runtime.udp_send_to(...)` stability `stable`
- `proc udp_close` signature `proc udp_close() -> int {` example `runtime.udp_close(...)` stability `stable`
- `proc udp_set_nonblocking` signature `proc udp_set_nonblocking() -> int {` example `runtime.udp_set_nonblocking(...)` stability `stable`
- `proc udp_set_read_timeout` signature `proc udp_set_read_timeout() -> int {` example `runtime.udp_set_read_timeout(...)` stability `stable`
- `proc udp_set_write_timeout` signature `proc udp_set_write_timeout() -> int {` example `runtime.udp_set_write_timeout(...)` stability `stable`
- `proc tcp_set_nonblocking` signature `proc tcp_set_nonblocking() -> int {` example `runtime.tcp_set_nonblocking(...)` stability `stable`
- `proc tcp_set_read_timeout` signature `proc tcp_set_read_timeout() -> int {` example `runtime.tcp_set_read_timeout(...)` stability `stable`
- `proc tcp_set_write_timeout` signature `proc tcp_set_write_timeout() -> int {` example `runtime.tcp_set_write_timeout(...)` stability `stable`
- `proc regex_compile` signature `proc regex_compile() -> int {` example `runtime.regex_compile(...)` stability `stable`
- `proc regex_is_match` signature `proc regex_is_match() -> int {` example `runtime.regex_is_match(...)` stability `stable`
- `proc regex_find` signature `proc regex_find() -> int {` example `runtime.regex_find(...)` stability `stable`
- `proc regex_replace` signature `proc regex_replace() -> int {` example `runtime.regex_replace(...)` stability `stable`
- `proc regex_split` signature `proc regex_split() -> int {` example `runtime.regex_split(...)` stability `stable`
- `proc fswatch_watch` signature `proc fswatch_watch() -> int {` example `runtime.fswatch_watch(...)` stability `stable`
- `proc fswatch_poll` signature `proc fswatch_poll() -> int {` example `runtime.fswatch_poll(...)` stability `stable`
- `proc fswatch_close` signature `proc fswatch_close() -> int {` example `runtime.fswatch_close(...)` stability `stable`
- `proc db_open` signature `proc db_open() -> int {` example `runtime.db_open(...)` stability `stable`
- `proc db_close` signature `proc db_close() -> int {` example `runtime.db_close(...)` stability `stable`
- `proc db_set` signature `proc db_set() -> int {` example `runtime.db_set(...)` stability `stable`
- `proc db_get` signature `proc db_get() -> int {` example `runtime.db_get(...)` stability `stable`
- `proc db_delete` signature `proc db_delete() -> int {` example `runtime.db_delete(...)` stability `stable`
- `proc db_keys` signature `proc db_keys() -> int {` example `runtime.db_keys(...)` stability `stable`
- `proc db_keys_prefix` signature `proc db_keys_prefix() -> int {` example `runtime.db_keys_prefix(...)` stability `stable`
- `proc db_batch_put` signature `proc db_batch_put() -> int {` example `runtime.db_batch_put(...)` stability `stable`
- `proc db_begin` signature `proc db_begin() -> int {` example `runtime.db_begin(...)` stability `stable`
- `proc db_commit` signature `proc db_commit() -> int {` example `runtime.db_commit(...)` stability `stable`
- `proc db_rollback` signature `proc db_rollback() -> int {` example `runtime.db_rollback(...)` stability `stable`
- `form RuntimeManifest` signature `form RuntimeManifest {` example `runtime.RuntimeManifest` stability `stable`
- `form RuntimeHealth` signature `form RuntimeHealth {` example `runtime.RuntimeHealth` stability `stable`
- `form RuntimeSummary` signature `form RuntimeSummary {` example `runtime.RuntimeSummary` stability `stable`
- `proc runtime_version` signature `proc runtime_version() -> string {` example `runtime.runtime_version(...)` stability `stable`
- `proc runtime_ready` signature `proc runtime_ready() -> bool {` example `runtime.runtime_ready(...)` stability `stable`
- `proc runtime_manifest` signature `proc runtime_manifest() -> RuntimeManifest {` example `runtime.runtime_manifest(...)` stability `stable`
- `proc runtime_health` signature `proc runtime_health() -> RuntimeHealth {` example `runtime.runtime_health(...)` stability `stable`
- `proc runtime_summary` signature `proc runtime_summary() -> RuntimeSummary {` example `runtime.runtime_summary(...)` stability `stable`
- `proc runtime_selftest` signature `proc runtime_selftest() -> bool {` example `runtime.runtime_selftest(...)` stability `stable`

## `src/vitte/stdlib/kernel.vitl`

Stability: `stable`

- `proc module_ready` signature `proc module_ready() -> bool {` example `kernel.module_ready(...)` stability `stable`

## `src/vitte/stdlib/graphics.vitl`

Stability: `stable`

- `proc graphics_version` signature `proc graphics_version() -> string {` example `graphics.graphics_version(...)` stability `stable`
- `proc create_canvas` signature `proc create_canvas(width: int, height: int) -> string {` example `graphics.create_canvas(...)` stability `stable`
- `proc save_canvas` signature `proc save_canvas(name: string) -> bool {` example `graphics.save_canvas(...)` stability `stable`
- `proc export_png` signature `proc export_png(canvas: string, filename: string) -> bool {` example `graphics.export_png(...)` stability `stable`
- `proc export_svg` signature `proc export_svg(canvas: string, filename: string) -> bool {` example `graphics.export_svg(...)` stability `stable`

## `src/vitte/stdlib/core/algorithms.vitl`

Stability: `stable`

- `pick SortOrder` signature `pick SortOrder {` example `algorithms.SortOrder` stability `stable`
- `form SearchResult` signature `form SearchResult {` example `algorithms.SearchResult` stability `stable`
- `proc compare_i64` signature `proc compare_i64(left: i64, right: i64, order: SortOrder) -> bool {` example `algorithms.compare_i64(...)` stability `stable`
- `proc swap_i64` signature `proc swap_i64(values: [i64], left: int, right: int) -> [i64] {` example `algorithms.swap_i64(...)` stability `stable`
- `proc reverse_i64` signature `proc reverse_i64(values: [i64]) -> [i64] {` example `algorithms.reverse_i64(...)` stability `stable`
- `proc bubble_sort_i64` signature `proc bubble_sort_i64(values: [i64], order: SortOrder) -> [i64] {` example `algorithms.bubble_sort_i64(...)` stability `stable`
- `proc insertion_sort_i64` signature `proc insertion_sort_i64(values: [i64], order: SortOrder) -> [i64] {` example `algorithms.insertion_sort_i64(...)` stability `stable`
- `proc selection_sort_i64` signature `proc selection_sort_i64(values: [i64], order: SortOrder) -> [i64] {` example `algorithms.selection_sort_i64(...)` stability `stable`
- `proc quick_sort_range_i64` signature `proc quick_sort_range_i64(values: [i64], low: int, high: int, order: SortOrder) -> [i64] {` example `algorithms.quick_sort_range_i64(...)` stability `stable`
- `proc quick_sort_i64` signature `proc quick_sort_i64(values: [i64], order: SortOrder) -> [i64] {` example `algorithms.quick_sort_i64(...)` stability `stable`
- `proc linear_search_i64` signature `proc linear_search_i64(values: [i64], target: i64) -> SearchResult {` example `algorithms.linear_search_i64(...)` stability `stable`
- `proc binary_search_i64` signature `proc binary_search_i64(values: [i64], target: i64) -> SearchResult {` example `algorithms.binary_search_i64(...)` stability `stable`
- `proc contains_i64` signature `proc contains_i64(values: [i64], target: i64) -> bool {` example `algorithms.contains_i64(...)` stability `stable`
- `proc sum_i64` signature `proc sum_i64(values: [i64]) -> i64 {` example `algorithms.sum_i64(...)` stability `stable`
- `proc average_i64` signature `proc average_i64(values: [i64]) -> f64 {` example `algorithms.average_i64(...)` stability `stable`
- `proc max_i64` signature `proc max_i64(values: [i64]) -> i64 {` example `algorithms.max_i64(...)` stability `stable`
- `proc min_i64` signature `proc min_i64(values: [i64]) -> i64 {` example `algorithms.min_i64(...)` stability `stable`
- `proc is_sorted_i64` signature `proc is_sorted_i64(values: [i64], order: SortOrder) -> bool {` example `algorithms.is_sorted_i64(...)` stability `stable`
- `proc unique_i64` signature `proc unique_i64(values: [i64]) -> [i64] {` example `algorithms.unique_i64(...)` stability `stable`
- `proc algorithms_selftest` signature `proc algorithms_selftest() -> bool {` example `algorithms.algorithms_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/array.vitl`

Stability: `stable`

- `form Array` signature `form Array<T> {` example `array.Array` stability `stable`
- `proc array_len` signature `proc array_len<T, const N: usize>(array: [T; N]) -> usize { give N; }` example `array.array_len(...)` stability `stable`
- `proc array_is_empty` signature `proc array_is_empty<T, const N: usize>(array: [T; N]) -> bool { give N == 0; }` example `array.array_is_empty(...)` stability `stable`
- `proc array_get` signature `proc array_get<T, const N: usize>(array: [T; N], index: usize) -> Option<ref T> { give compiler_array_get<T, N>(array, index); }` example `array.array_get(...)` stability `stable`
- `proc array_get_mut` signature `proc array_get_mut<T, const N: usize>(array: ref mut [T; N], index: usize) -> Option<ref mut T> { give compiler_array_get_mut<T, N>(array, index); }` example `array.array_get_mut(...)` stability `stable`
- `proc array_as_slice` signature `proc array_as_slice<T, const N: usize>(array: [T; N]) -> Slice<T> { give compiler_array_as_slice<T, N>(array); }` example `array.array_as_slice(...)` stability `stable`
- `proc array_iter` signature `proc array_iter<T, const N: usize>(array: [T; N]) -> Iterator<ref T> { give compiler_array_iter<T, N>(array); }` example `array.array_iter(...)` stability `stable`
- `proc array_map` signature `proc array_map<T, U, const N: usize>(array: [T; N], f: proc(T) -> U) -> [U; N] { give compiler_array_map<T, U, N>(array, f); }` example `array.array_map(...)` stability `stable`
- `proc array_fill` signature `proc array_fill<T, const N: usize>(value: T) -> [T; N] { give compiler_array_fill<T, N>(value); }` example `array.array_fill(...)` stability `stable`
- `proc array_copy` signature `proc array_copy<T, const N: usize>(array: [T; N]) -> [T; N] { give compiler_array_copy<T, N>(array); }` example `array.array_copy(...)` stability `stable`
- `proc array_sort` signature `proc array_sort<T, const N: usize>(array: ref mut [T; N], compare: proc(T, T) -> int) { compiler_array_sort<T, N>(array, compare); }` example `array.array_sort(...)` stability `stable`

## `src/vitte/stdlib/core/ascii.vitl`

Stability: `stable`

- `proc is_ascii` signature `proc is_ascii(value: byte) -> bool {` example `ascii.is_ascii(...)` stability `stable`
- `proc is_alphabetic` signature `proc is_alphabetic(value: byte) -> bool {` example `ascii.is_alphabetic(...)` stability `stable`
- `proc is_numeric` signature `proc is_numeric(value: byte) -> bool {` example `ascii.is_numeric(...)` stability `stable`
- `proc is_alphanumeric` signature `proc is_alphanumeric(value: byte) -> bool {` example `ascii.is_alphanumeric(...)` stability `stable`
- `proc is_whitespace` signature `proc is_whitespace(value: byte) -> bool {` example `ascii.is_whitespace(...)` stability `stable`
- `proc to_uppercase` signature `proc to_uppercase(value: byte) -> byte {` example `ascii.to_uppercase(...)` stability `stable`
- `proc to_lowercase` signature `proc to_lowercase(value: byte) -> byte {` example `ascii.to_lowercase(...)` stability `stable`
- `proc escape_ascii` signature `proc escape_ascii(value: byte) -> string {` example `ascii.escape_ascii(...)` stability `stable`

## `src/vitte/stdlib/core/clone.vitl`

Stability: `stable`

- `pick CloneCost` signature `pick CloneCost {` example `clone.CloneCost` stability `stable`
- `form Clone` signature `form Clone<T> {` example `clone.Clone` stability `stable`
- `form Copy` signature `form Copy<T> {` example `clone.Copy` stability `stable`
- `proc copy_trivial` signature `proc copy_trivial<T>(value: T) -> T {` example `clone.copy_trivial(...)` stability `stable`
- `proc clone_from_ref` signature `proc clone_from_ref<T>(value: ref T, clone_impl: Clone<T>) -> T {` example `clone.clone_from_ref(...)` stability `stable`
- `proc clone_cost_name` signature `proc clone_cost_name(cost: CloneCost) -> string {` example `clone.clone_cost_name(...)` stability `stable`
- `proc copy_is_trivial` signature `proc copy_is_trivial<T>(copy_impl: Copy<T>) -> bool {` example `clone.copy_is_trivial(...)` stability `stable`
- `proc clone_is_costly` signature `proc clone_is_costly<T>(clone_impl: Clone<T>) -> bool {` example `clone.clone_is_costly(...)` stability `stable`
- `proc implicit_clone_allowed` signature `proc implicit_clone_allowed<T>(clone_impl: Clone<T>) -> bool {` example `clone.implicit_clone_allowed(...)` stability `stable`
- `proc copy_forbidden_diagnostic` signature `proc copy_forbidden_diagnostic(type_name: string, reason: string) -> string {` example `clone.copy_forbidden_diagnostic(...)` stability `stable`
- `proc clone_required_diagnostic` signature `proc clone_required_diagnostic(type_name: string) -> string {` example `clone.clone_required_diagnostic(...)` stability `stable`
- `proc reject_implicit_clone` signature `proc reject_implicit_clone(type_name: string) -> string {` example `clone.reject_implicit_clone(...)` stability `stable`

## `src/vitte/stdlib/core/cmp.vitl`

Stability: `stable`

- `pick Ordering` signature `pick Ordering { Less Equal Greater }` example `cmp.Ordering` stability `stable`
- `form Eq` signature `form Eq<T> { eq: proc(T, T) -> bool }` example `cmp.Eq` stability `stable`
- `form PartialEq` signature `form PartialEq<T> { eq: proc(T, T) -> bool }` example `cmp.PartialEq` stability `stable`
- `form Ord` signature `form Ord<T> { cmp: proc(T, T) -> Ordering }` example `cmp.Ord` stability `stable`
- `form PartialOrd` signature `form PartialOrd<T> { partial_cmp: proc(T, T) -> Option<Ordering> }` example `cmp.PartialOrd` stability `stable`
- `proc compare` signature `proc compare<T>(left: T, right: T) -> Ordering { give compiler_cmp<T>(left, right); }` example `cmp.compare(...)` stability `stable`
- `proc equals` signature `proc equals<T>(left: T, right: T) -> bool { give compare<T>(left, right) == Ordering.Equal; }` example `cmp.equals(...)` stability `stable`
- `proc min` signature `proc min<T>(left: T, right: T) -> T { if compare<T>(left, right) == Ordering.Greater { give right; } give left; }` example `cmp.min(...)` stability `stable`
- `proc max` signature `proc max<T>(left: T, right: T) -> T { if compare<T>(left, right) == Ordering.Less { give right; } give left; }` example `cmp.max(...)` stability `stable`
- `proc clamp` signature `proc clamp<T>(value: T, low: T, high: T) -> T { give max<T>(low, min<T>(value, high)); }` example `cmp.clamp(...)` stability `stable`

## `src/vitte/stdlib/core/concurrency.vitl`

Stability: `stable`

- `pick TaskState` signature `pick TaskState {` example `concurrency.TaskState` stability `stable`
- `pick ChannelState` signature `pick ChannelState {` example `concurrency.ChannelState` stability `stable`
- `form Task` signature `form Task {` example `concurrency.Task` stability `stable`
- `form Mutex` signature `form Mutex {` example `concurrency.Mutex` stability `stable`
- `form SpinLock` signature `form SpinLock {` example `concurrency.SpinLock` stability `stable`
- `form Semaphore` signature `form Semaphore {` example `concurrency.Semaphore` stability `stable`
- `form Channel` signature `form Channel {` example `concurrency.Channel` stability `stable`
- `form RuntimeStats` signature `form RuntimeStats {` example `concurrency.RuntimeStats` stability `stable`
- `proc task` signature `proc task(task_id: u64, name: string) -> Task {` example `concurrency.task(...)` stability `stable`
- `proc mutex` signature `proc mutex() -> Mutex {` example `concurrency.mutex(...)` stability `stable`
- `proc spinlock` signature `proc spinlock() -> SpinLock {` example `concurrency.spinlock(...)` stability `stable`
- `proc semaphore` signature `proc semaphore(permits: int) -> Semaphore {` example `concurrency.semaphore(...)` stability `stable`
- `proc channel` signature `proc channel(channel_id: u64) -> Channel {` example `concurrency.channel(...)` stability `stable`
- `proc runtime_stats` signature `proc runtime_stats() -> RuntimeStats {` example `concurrency.runtime_stats(...)` stability `stable`
- `proc start_task` signature `proc start_task(task: Task) -> Task {` example `concurrency.start_task(...)` stability `stable`
- `proc wait_task` signature `proc wait_task(task: Task) -> Task {` example `concurrency.wait_task(...)` stability `stable`
- `proc complete_task` signature `proc complete_task(task: Task) -> Task {` example `concurrency.complete_task(...)` stability `stable`
- `proc fail_task` signature `proc fail_task(task: Task) -> Task {` example `concurrency.fail_task(...)` stability `stable`
- `proc cancel_task` signature `proc cancel_task(task: Task) -> Task {` example `concurrency.cancel_task(...)` stability `stable`
- `proc task_running` signature `proc task_running(task: Task) -> bool {` example `concurrency.task_running(...)` stability `stable`
- `proc task_finished` signature `proc task_finished(task: Task) -> bool {` example `concurrency.task_finished(...)` stability `stable`
- `proc lock_mutex` signature `proc lock_mutex(lock: Mutex, owner: u64) -> Mutex {` example `concurrency.lock_mutex(...)` stability `stable`
- `proc unlock_mutex` signature `proc unlock_mutex(lock: Mutex, owner: u64) -> Mutex {` example `concurrency.unlock_mutex(...)` stability `stable`
- `proc mutex_locked` signature `proc mutex_locked(lock: Mutex) -> bool {` example `concurrency.mutex_locked(...)` stability `stable`
- `proc acquire_spinlock` signature `proc acquire_spinlock(lock: SpinLock) -> SpinLock {` example `concurrency.acquire_spinlock(...)` stability `stable`
- `proc release_spinlock` signature `proc release_spinlock(lock: SpinLock) -> SpinLock {` example `concurrency.release_spinlock(...)` stability `stable`
- `proc spinlock_locked` signature `proc spinlock_locked(lock: SpinLock) -> bool {` example `concurrency.spinlock_locked(...)` stability `stable`
- `proc acquire_semaphore` signature `proc acquire_semaphore(sem: Semaphore) -> Semaphore {` example `concurrency.acquire_semaphore(...)` stability `stable`
- `proc release_semaphore` signature `proc release_semaphore(sem: Semaphore) -> Semaphore {` example `concurrency.release_semaphore(...)` stability `stable`
- `proc semaphore_available` signature `proc semaphore_available(sem: Semaphore) -> bool {` example `concurrency.semaphore_available(...)` stability `stable`
- `proc send_message` signature `proc send_message(ch: Channel, message: string) -> Channel {` example `concurrency.send_message(...)` stability `stable`
- `proc receive_message` signature `proc receive_message(ch: Channel) -> string {` example `concurrency.receive_message(...)` stability `stable`
- `proc pop_message` signature `proc pop_message(ch: Channel) -> Channel {` example `concurrency.pop_message(...)` stability `stable`
- `proc close_channel` signature `proc close_channel(ch: Channel) -> Channel {` example `concurrency.close_channel(...)` stability `stable`
- `proc channel_open` signature `proc channel_open(ch: Channel) -> bool {` example `concurrency.channel_open(...)` stability `stable`
- `proc count_running_tasks` signature `proc count_running_tasks(tasks: [Task]) -> int {` example `concurrency.count_running_tasks(...)` stability `stable`
- `proc count_waiting_tasks` signature `proc count_waiting_tasks(tasks: [Task]) -> int {` example `concurrency.count_waiting_tasks(...)` stability `stable`
- `proc count_completed_tasks` signature `proc count_completed_tasks(tasks: [Task]) -> int {` example `concurrency.count_completed_tasks(...)` stability `stable`
- `proc build_runtime_stats` signature `proc build_runtime_stats(tasks: [Task]) -> RuntimeStats {` example `concurrency.build_runtime_stats(...)` stability `stable`
- `proc concurrency_selftest` signature `proc concurrency_selftest() -> bool {` example `concurrency.concurrency_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/convert.vitl`

Stability: `stable`

- `pick ConversionKind` signature `pick ConversionKind {` example `convert.ConversionKind` stability `stable`
- `form ConversionResult` signature `form ConversionResult<T> {` example `convert.ConversionResult` stability `stable`
- `form From` signature `form From<T, U> {` example `convert.From` stability `stable`
- `form Into` signature `form Into<T, U> {` example `convert.Into` stability `stable`
- `form TryFrom` signature `form TryFrom<T, U, E> {` example `convert.TryFrom` stability `stable`
- `form TryInto` signature `form TryInto<T, U, E> {` example `convert.TryInto` stability `stable`
- `form AsRef` signature `form AsRef<T, U> {` example `convert.AsRef` stability `stable`
- `form AsMut` signature `form AsMut<T, U> {` example `convert.AsMut` stability `stable`
- `form Borrow` signature `form Borrow<T, U> {` example `convert.Borrow` stability `stable`
- `form BorrowMut` signature `form BorrowMut<T, U> {` example `convert.BorrowMut` stability `stable`
- `proc conversion_ok` signature `proc conversion_ok<T>(value: T) -> ConversionResult<T> {` example `convert.conversion_ok(...)` stability `stable`
- `proc conversion_error` signature `proc conversion_error<T>(message: string) -> ConversionResult<T> {` example `convert.conversion_error(...)` stability `stable`
- `proc numeric_safe_i32_to_i64` signature `proc numeric_safe_i32_to_i64(value: i32) -> i64 {` example `convert.numeric_safe_i32_to_i64(...)` stability `stable`
- `proc numeric_safe_u32_to_u64` signature `proc numeric_safe_u32_to_u64(value: u32) -> u64 {` example `convert.numeric_safe_u32_to_u64(...)` stability `stable`
- `proc numeric_checked_i64_to_i32` signature `proc numeric_checked_i64_to_i32(value: i64) -> ConversionResult<i32> {` example `convert.numeric_checked_i64_to_i32(...)` stability `stable`
- `proc numeric_checked_u64_to_u32` signature `proc numeric_checked_u64_to_u32(value: u64) -> ConversionResult<u32> {` example `convert.numeric_checked_u64_to_u32(...)` stability `stable`
- `proc numeric_saturating_i64_to_i32` signature `proc numeric_saturating_i64_to_i32(value: i64) -> i32 {` example `convert.numeric_saturating_i64_to_i32(...)` stability `stable`
- `proc numeric_saturating_u64_to_u32` signature `proc numeric_saturating_u64_to_u32(value: u64) -> u32 {` example `convert.numeric_saturating_u64_to_u32(...)` stability `stable`
- `proc numeric_wrapping_i64_to_i32` signature `proc numeric_wrapping_i64_to_i32(value: i64) -> i32 {` example `convert.numeric_wrapping_i64_to_i32(...)` stability `stable`
- `proc numeric_wrapping_u64_to_u32` signature `proc numeric_wrapping_u64_to_u32(value: u64) -> u32 {` example `convert.numeric_wrapping_u64_to_u32(...)` stability `stable`
- `proc numeric_truncating_f64_to_i64` signature `proc numeric_truncating_f64_to_i64(value: f64) -> i64 {` example `convert.numeric_truncating_f64_to_i64(...)` stability `stable`
- `proc numeric_truncating_f32_to_i32` signature `proc numeric_truncating_f32_to_i32(value: f32) -> i32 {` example `convert.numeric_truncating_f32_to_i32(...)` stability `stable`
- `proc dangerous_implicit_conversion_allowed` signature `proc dangerous_implicit_conversion_allowed(from_type: string, to_type: string) -> bool {` example `convert.dangerous_implicit_conversion_allowed(...)` stability `stable`
- `proc conversion_impossible_error` signature `proc conversion_impossible_error(from_type: string, to_type: string) -> string {` example `convert.conversion_impossible_error(...)` stability `stable`
- `proc require_explicit_conversion` signature `proc require_explicit_conversion(from_type: string, to_type: string, kind: ConversionKind) -> ConversionResult<string> {` example `convert.require_explicit_conversion(...)` stability `stable`

## `src/vitte/stdlib/core/default.vitl`

Stability: `stable`

- `form Default` signature `form Default<T> {` example `default.Default` stability `stable`
- `proc default_bool` signature `proc default_bool() -> bool {` example `default.default_bool(...)` stability `stable`
- `proc default_byte` signature `proc default_byte() -> byte {` example `default.default_byte(...)` stability `stable`
- `proc default_char` signature `proc default_char() -> char {` example `default.default_char(...)` stability `stable`
- `proc default_rune` signature `proc default_rune() -> rune {` example `default.default_rune(...)` stability `stable`
- `proc default_int` signature `proc default_int() -> int {` example `default.default_int(...)` stability `stable`
- `proc default_uint` signature `proc default_uint() -> uint {` example `default.default_uint(...)` stability `stable`
- `proc default_isize` signature `proc default_isize() -> isize {` example `default.default_isize(...)` stability `stable`
- `proc default_usize` signature `proc default_usize() -> usize {` example `default.default_usize(...)` stability `stable`
- `proc default_i8` signature `proc default_i8() -> i8 {` example `default.default_i8(...)` stability `stable`
- `proc default_i16` signature `proc default_i16() -> i16 {` example `default.default_i16(...)` stability `stable`
- `proc default_i32` signature `proc default_i32() -> i32 {` example `default.default_i32(...)` stability `stable`
- `proc default_i64` signature `proc default_i64() -> i64 {` example `default.default_i64(...)` stability `stable`
- `proc default_i128` signature `proc default_i128() -> i128 {` example `default.default_i128(...)` stability `stable`
- `proc default_u8` signature `proc default_u8() -> u8 {` example `default.default_u8(...)` stability `stable`
- `proc default_u16` signature `proc default_u16() -> u16 {` example `default.default_u16(...)` stability `stable`
- `proc default_u32` signature `proc default_u32() -> u32 {` example `default.default_u32(...)` stability `stable`
- `proc default_u64` signature `proc default_u64() -> u64 {` example `default.default_u64(...)` stability `stable`
- `proc default_u128` signature `proc default_u128() -> u128 {` example `default.default_u128(...)` stability `stable`
- `proc default_f32` signature `proc default_f32() -> f32 {` example `default.default_f32(...)` stability `stable`
- `proc default_f64` signature `proc default_f64() -> f64 {` example `default.default_f64(...)` stability `stable`
- `proc default_unit` signature `proc default_unit() -> unit {` example `default.default_unit(...)` stability `stable`
- `proc default_option` signature `proc default_option<T>() -> Option<T> {` example `default.default_option(...)` stability `stable`
- `proc default_result` signature `proc default_result<T, E>(error: E) -> Result<T, E> {` example `default.default_result(...)` stability `stable`
- `proc default_for_compatible_struct` signature `proc default_for_compatible_struct<T>(builder: proc() -> T, reason: string) -> Default<T> {` example `default.default_for_compatible_struct(...)` stability `stable`
- `proc default_is_meaningful` signature `proc default_is_meaningful<T>(definition: Default<T>) -> bool {` example `default.default_is_meaningful(...)` stability `stable`
- `proc reject_meaningless_default` signature `proc reject_meaningless_default(type_name: string) -> string {` example `default.reject_meaningless_default(...)` stability `stable`

## `src/vitte/stdlib/core/drop.vitl`

Stability: `stable`

- `pick DropState` signature `pick DropState {` example `drop.DropState` stability `stable`
- `pick DropOrder` signature `pick DropOrder {` example `drop.DropOrder` stability `stable`
- `form DropProtocol` signature `form DropProtocol<T> {` example `drop.DropProtocol` stability `stable`
- `form DropResult` signature `form DropResult {` example `drop.DropResult` stability `stable`
- `form DropSlot` signature `form DropSlot {` example `drop.DropSlot` stability `stable`
- `proc drop_ok` signature `proc drop_ok(state: DropState) -> DropResult {` example `drop.drop_ok(...)` stability `stable`
- `proc drop_error` signature `proc drop_error(message: string) -> DropResult {` example `drop.drop_error(...)` stability `stable`
- `proc drop` signature `proc drop<T>(value: ref mut T, protocol: DropProtocol<T>) -> DropResult {` example `drop.drop(...)` stability `stable`
- `proc forget` signature `proc forget<T>(value: T) -> DropResult {` example `drop.forget(...)` stability `stable`
- `proc needs_drop` signature `proc needs_drop<T>(protocol: DropProtocol<T>) -> bool {` example `drop.needs_drop(...)` stability `stable`
- `proc drop_slot` signature `proc drop_slot(name: string, sequence: u64) -> DropSlot {` example `drop.drop_slot(...)` stability `stable`
- `proc mark_dropped` signature `proc mark_dropped(slot: DropSlot) -> DropSlot {` example `drop.mark_dropped(...)` stability `stable`
- `proc mark_partial` signature `proc mark_partial(slot: DropSlot) -> DropSlot {` example `drop.mark_partial(...)` stability `stable`
- `proc deterministic_drop_order` signature `proc deterministic_drop_order(left: DropSlot, right: DropSlot) -> bool {` example `drop.deterministic_drop_order(...)` stability `stable`
- `proc partial_drop_allowed` signature `proc partial_drop_allowed(slot: DropSlot) -> bool {` example `drop.partial_drop_allowed(...)` stability `stable`
- `proc early_exit_drop_required` signature `proc early_exit_drop_required(slot: DropSlot) -> bool {` example `drop.early_exit_drop_required(...)` stability `stable`
- `proc drop_error_policy` signature `proc drop_error_policy(result: DropResult) -> DropResult {` example `drop.drop_error_policy(...)` stability `stable`
- `proc double_drop_forbidden` signature `proc double_drop_forbidden(slot: DropSlot) -> bool {` example `drop.double_drop_forbidden(...)` stability `stable`
- `proc double_drop_diagnostic` signature `proc double_drop_diagnostic(name: string) -> string {` example `drop.double_drop_diagnostic(...)` stability `stable`

## `src/vitte/stdlib/core/float.vitl`

Stability: `stable`

- `pick FloatClass` signature `pick FloatClass {` example `float.FloatClass` stability `stable`
- `pick FloatOrdering` signature `pick FloatOrdering {` example `float.FloatOrdering` stability `stable`
- `proc is_nan` signature `proc is_nan<T>(value: T) -> bool {` example `float.is_nan(...)` stability `stable`
- `proc is_infinite` signature `proc is_infinite<T>(value: T) -> bool {` example `float.is_infinite(...)` stability `stable`
- `proc is_finite` signature `proc is_finite<T>(value: T) -> bool {` example `float.is_finite(...)` stability `stable`
- `proc is_normal` signature `proc is_normal<T>(value: T) -> bool {` example `float.is_normal(...)` stability `stable`
- `proc is_subnormal` signature `proc is_subnormal<T>(value: T) -> bool {` example `float.is_subnormal(...)` stability `stable`
- `proc is_sign_positive` signature `proc is_sign_positive<T>(value: T) -> bool {` example `float.is_sign_positive(...)` stability `stable`
- `proc is_sign_negative` signature `proc is_sign_negative<T>(value: T) -> bool {` example `float.is_sign_negative(...)` stability `stable`
- `proc next_up` signature `proc next_up<T>(value: T) -> T {` example `float.next_up(...)` stability `stable`
- `proc next_down` signature `proc next_down<T>(value: T) -> T {` example `float.next_down(...)` stability `stable`
- `proc total_compare` signature `proc total_compare<T>(left: T, right: T) -> FloatOrdering {` example `float.total_compare(...)` stability `stable`
- `proc float_class` signature `proc float_class<T>(value: T) -> FloatClass {` example `float.float_class(...)` stability `stable`

## `src/vitte/stdlib/core/hash.vitl`

Stability: `stable`

- `form Hasher` signature `form Hasher {` example `hash.Hasher` stability `stable`
- `form Hash` signature `form Hash<T> {` example `hash.Hash` stability `stable`
- `proc hasher` signature `proc hasher(seed: u64) -> Hasher { give Hasher { state: seed }; }` example `hash.hasher(...)` stability `stable`
- `proc write_u64` signature `proc write_u64(h: ref mut Hasher, value: u64) { compiler_hash_write_u64(h, value); }` example `hash.write_u64(...)` stability `stable`
- `proc write_bytes` signature `proc write_bytes(h: ref mut Hasher, values: [byte]) { compiler_hash_write_bytes(h, values); }` example `hash.write_bytes(...)` stability `stable`
- `proc finish` signature `proc finish(h: Hasher) -> u64 { give compiler_hash_finish(h); }` example `hash.finish(...)` stability `stable`
- `proc hash` signature `proc hash<T>(value: T) -> u64 { give compiler_hash<T>(value); }` example `hash.hash(...)` stability `stable`
- `proc combine_hash` signature `proc combine_hash(left: u64, right: u64) -> u64 { give compiler_hash_combine(left, right); }` example `hash.combine_hash(...)` stability `stable`

## `src/vitte/stdlib/core/io_helpers.vitl`

Stability: `stable`

- `pick IoStatus` signature `pick IoStatus {` example `io_helpers.IoStatus` stability `stable`
- `form IoResult` signature `form IoResult {` example `io_helpers.IoResult` stability `stable`
- `form ReadResult` signature `form ReadResult {` example `io_helpers.ReadResult` stability `stable`
- `form WriteResult` signature `form WriteResult {` example `io_helpers.WriteResult` stability `stable`
- `form FileInfo` signature `form FileInfo {` example `io_helpers.FileInfo` stability `stable`
- `proc ok_result` signature `proc ok_result(message: string) -> IoResult {` example `io_helpers.ok_result(...)` stability `stable`
- `proc error_result` signature `proc error_result(status: IoStatus, message: string) -> IoResult {` example `io_helpers.error_result(...)` stability `stable`
- `proc empty_read_result` signature `proc empty_read_result(status: IoStatus) -> ReadResult {` example `io_helpers.empty_read_result(...)` stability `stable`
- `proc empty_write_result` signature `proc empty_write_result(status: IoStatus) -> WriteResult {` example `io_helpers.empty_write_result(...)` stability `stable`
- `proc file_info` signature `proc file_info(path: string) -> FileInfo {` example `io_helpers.file_info(...)` stability `stable`
- `proc read_text` signature `proc read_text(path: string) -> ReadResult {` example `io_helpers.read_text(...)` stability `stable`
- `proc write_text` signature `proc write_text(path: string, data: string) -> WriteResult {` example `io_helpers.write_text(...)` stability `stable`
- `proc append_text` signature `proc append_text(path: string, data: string) -> WriteResult {` example `io_helpers.append_text(...)` stability `stable`
- `proc read_lines` signature `proc read_lines(path: string) -> [string] {` example `io_helpers.read_lines(...)` stability `stable`
- `proc write_lines` signature `proc write_lines(path: string, lines: [string]) -> WriteResult {` example `io_helpers.write_lines(...)` stability `stable`
- `proc exists` signature `proc exists(path: string) -> bool {` example `io_helpers.exists(...)` stability `stable`
- `proc readable` signature `proc readable(path: string) -> bool {` example `io_helpers.readable(...)` stability `stable`
- `proc writable` signature `proc writable(path: string) -> bool {` example `io_helpers.writable(...)` stability `stable`
- `proc basename` signature `proc basename(path: string) -> string {` example `io_helpers.basename(...)` stability `stable`
- `proc dirname` signature `proc dirname(path: string) -> string {` example `io_helpers.dirname(...)` stability `stable`
- `proc extension` signature `proc extension(path: string) -> string {` example `io_helpers.extension(...)` stability `stable`
- `proc join_path` signature `proc join_path(left: string, right: string) -> string {` example `io_helpers.join_path(...)` stability `stable`
- `proc normalize_newlines` signature `proc normalize_newlines(text: string) -> string {` example `io_helpers.normalize_newlines(...)` stability `stable`
- `proc read_bytes` signature `proc read_bytes(path: string) -> [int] {` example `io_helpers.read_bytes(...)` stability `stable`
- `proc write_bytes` signature `proc write_bytes(path: string, data: [int]) -> WriteResult {` example `io_helpers.write_bytes(...)` stability `stable`
- `proc io_helpers_selftest` signature `proc io_helpers_selftest() -> bool {` example `io_helpers.io_helpers_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/iterator.vitl`

Stability: `stable`

- `pick IteratorKind` signature `pick IteratorKind {` example `iterator.IteratorKind` stability `stable`
- `form SizeHint` signature `form SizeHint {` example `iterator.SizeHint` stability `stable`
- `form Iterator` signature `form Iterator<T> {` example `iterator.Iterator` stability `stable`
- `form DoubleEndedIterator` signature `form DoubleEndedIterator<T> {` example `iterator.DoubleEndedIterator` stability `stable`
- `form ExactSizeIterator` signature `form ExactSizeIterator<T> {` example `iterator.ExactSizeIterator` stability `stable`
- `form FusedIterator` signature `form FusedIterator<T> {` example `iterator.FusedIterator` stability `stable`
- `form CloneableIterator` signature `form CloneableIterator<T> {` example `iterator.CloneableIterator` stability `stable`
- `form RangeIterator` signature `form RangeIterator<T> {` example `iterator.RangeIterator` stability `stable`
- `form ChainIterator` signature `form ChainIterator<T> {` example `iterator.ChainIterator` stability `stable`
- `form ZipIterator` signature `form ZipIterator<T, U> {` example `iterator.ZipIterator` stability `stable`
- `form MapIterator` signature `form MapIterator<T, U> {` example `iterator.MapIterator` stability `stable`
- `form FilterIterator` signature `form FilterIterator<T> {` example `iterator.FilterIterator` stability `stable`
- `form PeekableIterator` signature `form PeekableIterator<T> {` example `iterator.PeekableIterator` stability `stable`
- `proc size_hint` signature `proc size_hint(lower: usize, upper: Option<usize>) -> SizeHint {` example `iterator.size_hint(...)` stability `stable`
- `proc next` signature `proc next<T>(iter: ref mut Iterator<T>) -> Option<T> {` example `iterator.next(...)` stability `stable`
- `proc iterator_size_hint` signature `proc iterator_size_hint<T>(iter: Iterator<T>) -> SizeHint {` example `iterator.iterator_size_hint(...)` stability `stable`
- `proc count` signature `proc count<T>(iter: Iterator<T>) -> usize {` example `iterator.count(...)` stability `stable`
- `proc last` signature `proc last<T>(iter: Iterator<T>) -> Option<T> {` example `iterator.last(...)` stability `stable`
- `proc nth` signature `proc nth<T>(iter: ref mut Iterator<T>, index: usize) -> Option<T> {` example `iterator.nth(...)` stability `stable`
- `proc step_by` signature `proc step_by<T>(iter: Iterator<T>, step: usize) -> Iterator<T> {` example `iterator.step_by(...)` stability `stable`
- `proc chain` signature `proc chain<T>(first: Iterator<T>, second: Iterator<T>) -> ChainIterator<T> {` example `iterator.chain(...)` stability `stable`
- `proc zip` signature `proc zip<T, U>(left: Iterator<T>, right: Iterator<U>) -> ZipIterator<T, U> {` example `iterator.zip(...)` stability `stable`
- `proc map` signature `proc map<T, U>(iter: Iterator<T>, f: proc(T) -> U) -> MapIterator<T, U> {` example `iterator.map(...)` stability `stable`
- `proc filter` signature `proc filter<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> FilterIterator<T> {` example `iterator.filter(...)` stability `stable`
- `proc filter_map` signature `proc filter_map<T, U>(iter: Iterator<T>, f: proc(T) -> Option<U>) -> Iterator<U> {` example `iterator.filter_map(...)` stability `stable`
- `proc flat_map` signature `proc flat_map<T, U>(iter: Iterator<T>, f: proc(T) -> Iterator<U>) -> Iterator<U> {` example `iterator.flat_map(...)` stability `stable`
- `proc flatten` signature `proc flatten<T>(iter: Iterator<Iterator<T>>) -> Iterator<T> {` example `iterator.flatten(...)` stability `stable`
- `proc enumerate` signature `proc enumerate<T>(iter: Iterator<T>) -> Iterator<(usize, T)> {` example `iterator.enumerate(...)` stability `stable`
- `proc peekable` signature `proc peekable<T>(iter: Iterator<T>) -> PeekableIterator<T> {` example `iterator.peekable(...)` stability `stable`
- `proc skip` signature `proc skip<T>(iter: Iterator<T>, amount: usize) -> Iterator<T> {` example `iterator.skip(...)` stability `stable`
- `proc take` signature `proc take<T>(iter: Iterator<T>, amount: usize) -> Iterator<T> {` example `iterator.take(...)` stability `stable`
- `proc skip_while` signature `proc skip_while<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> Iterator<T> {` example `iterator.skip_while(...)` stability `stable`
- `proc take_while` signature `proc take_while<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> Iterator<T> {` example `iterator.take_while(...)` stability `stable`
- `proc scan` signature `proc scan<T, S, U>(iter: Iterator<T>, state: S, f: proc(ref mut S, T) -> Option<U>) -> Iterator<U> {` example `iterator.scan(...)` stability `stable`
- `proc inspect` signature `proc inspect<T>(iter: Iterator<T>, f: proc(T)) -> Iterator<T> {` example `iterator.inspect(...)` stability `stable`
- `proc fold` signature `proc fold<T, U>(iter: Iterator<T>, init: U, f: proc(U, T) -> U) -> U {` example `iterator.fold(...)` stability `stable`
- `proc reduce` signature `proc reduce<T>(iter: Iterator<T>, f: proc(T, T) -> T) -> Option<T> {` example `iterator.reduce(...)` stability `stable`
- `proc try_fold` signature `proc try_fold<T, U, E>(iter: Iterator<T>, init: U, f: proc(U, T) -> Result<U, E>) -> Result<U, E> {` example `iterator.try_fold(...)` stability `stable`
- `proc all` signature `proc all<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> bool {` example `iterator.all(...)` stability `stable`
- `proc any` signature `proc any<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> bool {` example `iterator.any(...)` stability `stable`
- `proc find` signature `proc find<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> Option<T> {` example `iterator.find(...)` stability `stable`
- `proc find_map` signature `proc find_map<T, U>(iter: Iterator<T>, f: proc(T) -> Option<U>) -> Option<U> {` example `iterator.find_map(...)` stability `stable`
- `proc position` signature `proc position<T>(iter: Iterator<T>, predicate: proc(T) -> bool) -> Option<usize> {` example `iterator.position(...)` stability `stable`
- `proc rposition` signature `proc rposition<T>(iter: DoubleEndedIterator<T>, predicate: proc(T) -> bool) -> Option<usize> {` example `iterator.rposition(...)` stability `stable`
- `proc min` signature `proc min<T>(iter: Iterator<T>) -> Option<T> {` example `iterator.min(...)` stability `stable`
- `proc max` signature `proc max<T>(iter: Iterator<T>) -> Option<T> {` example `iterator.max(...)` stability `stable`
- `proc min_by` signature `proc min_by<T>(iter: Iterator<T>, compare: proc(T, T) -> int) -> Option<T> {` example `iterator.min_by(...)` stability `stable`
- `proc max_by` signature `proc max_by<T>(iter: Iterator<T>, compare: proc(T, T) -> int) -> Option<T> {` example `iterator.max_by(...)` stability `stable`
- `proc sum` signature `proc sum<T>(iter: Iterator<T>) -> T {` example `iterator.sum(...)` stability `stable`
- `proc product` signature `proc product<T>(iter: Iterator<T>) -> T {` example `iterator.product(...)` stability `stable`
- `proc collect` signature `proc collect<T, C>(iter: Iterator<T>) -> C {` example `iterator.collect(...)` stability `stable`
- `proc partition` signature `proc partition<T, C>(iter: Iterator<T>, predicate: proc(T) -> bool) -> (C, C) {` example `iterator.partition(...)` stability `stable`
- `proc unzip` signature `proc unzip<T, U, C, D>(iter: Iterator<(T, U)>) -> (C, D) {` example `iterator.unzip(...)` stability `stable`
- `proc double_ended` signature `proc double_ended<T>(iter: Iterator<T>, next_back_item: proc(ref mut DoubleEndedIterator<T>) -> Option<T>) -> DoubleEndedIterator<T> {` example `iterator.double_ended(...)` stability `stable`
- `proc exact_size` signature `proc exact_size<T>(iter: Iterator<T>, length: usize) -> ExactSizeIterator<T> {` example `iterator.exact_size(...)` stability `stable`
- `proc fused` signature `proc fused<T>(iter: Iterator<T>) -> FusedIterator<T> {` example `iterator.fused(...)` stability `stable`
- `proc cloneable` signature `proc cloneable<T>(iter: Iterator<T>, clone_iter: proc(CloneableIterator<T>) -> CloneableIterator<T>) -> CloneableIterator<T> {` example `iterator.cloneable(...)` stability `stable`
- `proc range` signature `proc range<T>(start: T, end: T, step: T) -> RangeIterator<T> {` example `iterator.range(...)` stability `stable`

## `src/vitte/stdlib/core/math.vitl`

Stability: `stable`

- `const PI` signature `const PI: f64 = 3.141592653589793` example `math.PI` stability `stable`
- `const TAU` signature `const TAU: f64 = 6.283185307179586` example `math.TAU` stability `stable`
- `const E` signature `const E: f64 = 2.718281828459045` example `math.E` stability `stable`
- `const FRAC_PI_2` signature `const FRAC_PI_2: f64 = 1.5707963267948966` example `math.FRAC_PI_2` stability `stable`
- `const FRAC_PI_3` signature `const FRAC_PI_3: f64 = 1.0471975511965979` example `math.FRAC_PI_3` stability `stable`
- `const FRAC_PI_4` signature `const FRAC_PI_4: f64 = 0.7853981633974483` example `math.FRAC_PI_4` stability `stable`
- `const FRAC_PI_6` signature `const FRAC_PI_6: f64 = 0.5235987755982989` example `math.FRAC_PI_6` stability `stable`
- `const FRAC_PI_8` signature `const FRAC_PI_8: f64 = 0.39269908169872414` example `math.FRAC_PI_8` stability `stable`
- `const SQRT_2` signature `const SQRT_2: f64 = 1.4142135623730951` example `math.SQRT_2` stability `stable`
- `const FRAC_1_SQRT_2` signature `const FRAC_1_SQRT_2: f64 = 0.7071067811865476` example `math.FRAC_1_SQRT_2` stability `stable`
- `const LN_2` signature `const LN_2: f64 = 0.6931471805599453` example `math.LN_2` stability `stable`
- `const LN_10` signature `const LN_10: f64 = 2.302585092994046` example `math.LN_10` stability `stable`
- `const LOG2_E` signature `const LOG2_E: f64 = 1.4426950408889634` example `math.LOG2_E` stability `stable`
- `const LOG10_E` signature `const LOG10_E: f64 = 0.4342944819032518` example `math.LOG10_E` stability `stable`
- `proc backend_math_available` signature `proc backend_math_available(operation: string) -> bool {` example `math.backend_math_available(...)` stability `stable`
- `proc sqrt` signature `proc sqrt(value: f64) -> f64 {` example `math.sqrt(...)` stability `stable`
- `proc cbrt` signature `proc cbrt(value: f64) -> f64 {` example `math.cbrt(...)` stability `stable`
- `proc hypot` signature `proc hypot(left: f64, right: f64) -> f64 {` example `math.hypot(...)` stability `stable`
- `proc exp` signature `proc exp(value: f64) -> f64 {` example `math.exp(...)` stability `stable`
- `proc exp2` signature `proc exp2(value: f64) -> f64 {` example `math.exp2(...)` stability `stable`
- `proc expm1` signature `proc expm1(value: f64) -> f64 {` example `math.expm1(...)` stability `stable`
- `proc ln` signature `proc ln(value: f64) -> f64 {` example `math.ln(...)` stability `stable`
- `proc ln1p` signature `proc ln1p(value: f64) -> f64 {` example `math.ln1p(...)` stability `stable`
- `proc log2` signature `proc log2(value: f64) -> f64 {` example `math.log2(...)` stability `stable`
- `proc log10` signature `proc log10(value: f64) -> f64 {` example `math.log10(...)` stability `stable`
- `proc sin` signature `proc sin(value: f64) -> f64 {` example `math.sin(...)` stability `stable`
- `proc cos` signature `proc cos(value: f64) -> f64 {` example `math.cos(...)` stability `stable`
- `proc tan` signature `proc tan(value: f64) -> f64 {` example `math.tan(...)` stability `stable`
- `proc asin` signature `proc asin(value: f64) -> f64 {` example `math.asin(...)` stability `stable`
- `proc acos` signature `proc acos(value: f64) -> f64 {` example `math.acos(...)` stability `stable`
- `proc atan` signature `proc atan(value: f64) -> f64 {` example `math.atan(...)` stability `stable`
- `proc atan2` signature `proc atan2(y: f64, x: f64) -> f64 {` example `math.atan2(...)` stability `stable`
- `proc sinh` signature `proc sinh(value: f64) -> f64 {` example `math.sinh(...)` stability `stable`
- `proc cosh` signature `proc cosh(value: f64) -> f64 {` example `math.cosh(...)` stability `stable`
- `proc tanh` signature `proc tanh(value: f64) -> f64 {` example `math.tanh(...)` stability `stable`
- `proc floor` signature `proc floor(value: f64) -> f64 {` example `math.floor(...)` stability `stable`
- `proc ceil` signature `proc ceil(value: f64) -> f64 {` example `math.ceil(...)` stability `stable`
- `proc round` signature `proc round(value: f64) -> f64 {` example `math.round(...)` stability `stable`
- `proc trunc` signature `proc trunc(value: f64) -> f64 {` example `math.trunc(...)` stability `stable`
- `proc fract` signature `proc fract(value: f64) -> f64 {` example `math.fract(...)` stability `stable`
- `proc copysign` signature `proc copysign(magnitude: f64, sign_value: f64) -> f64 {` example `math.copysign(...)` stability `stable`
- `proc fma` signature `proc fma(a: f64, b: f64, c: f64) -> f64 {` example `math.fma(...)` stability `stable`
- `proc portable_math_sqrt` signature `proc portable_math_sqrt(value: f64) -> f64 {` example `math.portable_math_sqrt(...)` stability `stable`
- `proc portable_math_cbrt` signature `proc portable_math_cbrt(value: f64) -> f64 {` example `math.portable_math_cbrt(...)` stability `stable`
- `proc portable_math_hypot` signature `proc portable_math_hypot(left: f64, right: f64) -> f64 {` example `math.portable_math_hypot(...)` stability `stable`
- `proc portable_math_exp` signature `proc portable_math_exp(value: f64) -> f64 {` example `math.portable_math_exp(...)` stability `stable`
- `proc portable_math_exp2` signature `proc portable_math_exp2(value: f64) -> f64 {` example `math.portable_math_exp2(...)` stability `stable`
- `proc portable_math_ln` signature `proc portable_math_ln(value: f64) -> f64 {` example `math.portable_math_ln(...)` stability `stable`
- `proc portable_math_sin` signature `proc portable_math_sin(value: f64) -> f64 {` example `math.portable_math_sin(...)` stability `stable`
- `proc portable_math_cos` signature `proc portable_math_cos(value: f64) -> f64 {` example `math.portable_math_cos(...)` stability `stable`
- `proc portable_math_asin` signature `proc portable_math_asin(value: f64) -> f64 {` example `math.portable_math_asin(...)` stability `stable`
- `proc portable_math_atan` signature `proc portable_math_atan(value: f64) -> f64 {` example `math.portable_math_atan(...)` stability `stable`
- `proc portable_math_atan2` signature `proc portable_math_atan2(y: f64, x: f64) -> f64 {` example `math.portable_math_atan2(...)` stability `stable`
- `proc portable_math_floor` signature `proc portable_math_floor(value: f64) -> f64 {` example `math.portable_math_floor(...)` stability `stable`
- `proc portable_math_ceil` signature `proc portable_math_ceil(value: f64) -> f64 {` example `math.portable_math_ceil(...)` stability `stable`
- `proc portable_math_round` signature `proc portable_math_round(value: f64) -> f64 {` example `math.portable_math_round(...)` stability `stable`
- `proc portable_math_trunc` signature `proc portable_math_trunc(value: f64) -> f64 {` example `math.portable_math_trunc(...)` stability `stable`
- `proc portable_math_copysign` signature `proc portable_math_copysign(magnitude: f64, sign_value: f64) -> f64 {` example `math.portable_math_copysign(...)` stability `stable`

## `src/vitte/stdlib/core/memory.vitl`

Stability: `stable`

- `pick AllocationState` signature `pick AllocationState {` example `memory.AllocationState` stability `stable`
- `form MemoryBlock` signature `form MemoryBlock {` example `memory.MemoryBlock` stability `stable`
- `form MemoryStats` signature `form MemoryStats {` example `memory.MemoryStats` stability `stable`
- `form Allocator` signature `form Allocator {` example `memory.Allocator` stability `stable`
- `form MemoryLayout` signature `form MemoryLayout {` example `memory.MemoryLayout` stability `stable`
- `form MemoryCheck` signature `form MemoryCheck {` example `memory.MemoryCheck` stability `stable`
- `proc empty_memory_stats` signature `proc empty_memory_stats() -> MemoryStats {` example `memory.empty_memory_stats(...)` stability `stable`
- `proc empty_allocator` signature `proc empty_allocator() -> Allocator {` example `memory.empty_allocator(...)` stability `stable`
- `proc memory_block` signature `proc memory_block(address: u64, size: u64) -> MemoryBlock {` example `memory.memory_block(...)` stability `stable`
- `proc reserved_block` signature `proc reserved_block(address: u64, size: u64) -> MemoryBlock {` example `memory.reserved_block(...)` stability `stable`
- `proc free_block` signature `proc free_block(address: u64, size: u64) -> MemoryBlock {` example `memory.free_block(...)` stability `stable`
- `proc allocator` signature `proc allocator(total: u64) -> Allocator {` example `memory.allocator(...)` stability `stable`
- `proc allocate` signature `proc allocate(alloc: Allocator, address: u64, size: u64) -> Allocator {` example `memory.allocate(...)` stability `stable`
- `proc reserve` signature `proc reserve(alloc: Allocator, address: u64, size: u64) -> Allocator {` example `memory.reserve(...)` stability `stable`
- `proc release` signature `proc release(alloc: Allocator, address: u64) -> Allocator {` example `memory.release(...)` stability `stable`
- `proc find_block` signature `proc find_block(alloc: Allocator, address: u64) -> MemoryBlock {` example `memory.find_block(...)` stability `stable`
- `proc allocated` signature `proc allocated(alloc: Allocator, address: u64) -> bool {` example `memory.allocated(...)` stability `stable`
- `proc reserved` signature `proc reserved(alloc: Allocator, address: u64) -> bool {` example `memory.reserved(...)` stability `stable`
- `proc total_used` signature `proc total_used(alloc: Allocator) -> u64 {` example `memory.total_used(...)` stability `stable`
- `proc total_free` signature `proc total_free(alloc: Allocator) -> u64 {` example `memory.total_free(...)` stability `stable`
- `proc allocation_count` signature `proc allocation_count(alloc: Allocator) -> u64 {` example `memory.allocation_count(...)` stability `stable`
- `proc free_count` signature `proc free_count(alloc: Allocator) -> u64 {` example `memory.free_count(...)` stability `stable`
- `proc fragmentation_ratio` signature `proc fragmentation_ratio(alloc: Allocator) -> f64 {` example `memory.fragmentation_ratio(...)` stability `stable`
- `proc clear_allocator` signature `proc clear_allocator(alloc: Allocator) -> Allocator {` example `memory.clear_allocator(...)` stability `stable`
- `proc clone_allocator` signature `proc clone_allocator(alloc: Allocator) -> Allocator {` example `memory.clone_allocator(...)` stability `stable`
- `proc memory_layout` signature `proc memory_layout(size: u64, align: u64) -> MemoryLayout {` example `memory.memory_layout(...)` stability `stable`
- `proc memory_check_ok` signature `proc memory_check_ok() -> MemoryCheck {` example `memory.memory_check_ok(...)` stability `stable`
- `proc memory_check_error` signature `proc memory_check_error(message: string) -> MemoryCheck {` example `memory.memory_check_error(...)` stability `stable`
- `proc size_of` signature `proc size_of<T>() -> u64 {` example `memory.size_of(...)` stability `stable`
- `proc size_of_value` signature `proc size_of_value<T>(value: ref T) -> u64 {` example `memory.size_of_value(...)` stability `stable`
- `proc align_of` signature `proc align_of<T>() -> u64 {` example `memory.align_of(...)` stability `stable`
- `proc align_of_value` signature `proc align_of_value<T>(value: ref T) -> u64 {` example `memory.align_of_value(...)` stability `stable`
- `proc offset_of` signature `proc offset_of<T>(field_name: string) -> u64 {` example `memory.offset_of(...)` stability `stable`
- `proc swap` signature `proc swap<T>(left: ref mut T, right: ref mut T) {` example `memory.swap(...)` stability `stable`
- `proc replace` signature `proc replace<T>(slot: ref mut T, value: T) -> T {` example `memory.replace(...)` stability `stable`
- `proc take` signature `proc take<T>(slot: ref mut T, default_value: T) -> T {` example `memory.take(...)` stability `stable`
- `proc forget` signature `proc forget<T>(value: T) {` example `memory.forget(...)` stability `stable`
- `proc drop` signature `proc drop<T>(value: ref mut T) {` example `memory.drop(...)` stability `stable`
- `proc detect_size_compatible` signature `proc detect_size_compatible(from_layout: MemoryLayout, to_layout: MemoryLayout) -> MemoryCheck {` example `memory.detect_size_compatible(...)` stability `stable`
- `proc detect_alignment_compatible` signature `proc detect_alignment_compatible(from_layout: MemoryLayout, to_layout: MemoryLayout) -> MemoryCheck {` example `memory.detect_alignment_compatible(...)` stability `stable`
- `proc unsafe_transmute` signature `proc unsafe_transmute<T, U>(value: T, from_layout: MemoryLayout, to_layout: MemoryLayout) -> U {` example `memory.unsafe_transmute(...)` stability `stable`
- `proc unsafe_zeroed` signature `proc unsafe_zeroed<T>() -> T {` example `memory.unsafe_zeroed(...)` stability `stable`
- `proc unsafe_uninitialized` signature `proc unsafe_uninitialized<T>() -> T {` example `memory.unsafe_uninitialized(...)` stability `stable`
- `proc unsafe_operation_invariants` signature `proc unsafe_operation_invariants(operation: string) -> string {` example `memory.unsafe_operation_invariants(...)` stability `stable`
- `proc memory_selftest` signature `proc memory_selftest() -> bool {` example `memory.memory_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/number.vitl`

Stability: `stable`

- `form NumberLimits` signature `form NumberLimits<T> {` example `number.NumberLimits` stability `stable`
- `form FloatConstants` signature `form FloatConstants<T> {` example `number.FloatConstants` stability `stable`
- `form Checked` signature `form Checked<T> {` example `number.Checked` stability `stable`
- `form Overflowing` signature `form Overflowing<T> {` example `number.Overflowing` stability `stable`
- `pick Sign` signature `pick Sign {` example `number.Sign` stability `stable`
- `pick Endian` signature `pick Endian {` example `number.Endian` stability `stable`
- `proc number_limits` signature `proc number_limits<T>() -> NumberLimits<T> {` example `number.number_limits(...)` stability `stable`
- `proc float_constants` signature `proc float_constants<T>() -> FloatConstants<T> {` example `number.float_constants(...)` stability `stable`
- `proc checked` signature `proc checked<T>(ok: bool, value: T) -> Checked<T> {` example `number.checked(...)` stability `stable`
- `proc checked_add` signature `proc checked_add<T>(left: T, right: T) -> Checked<T> {` example `number.checked_add(...)` stability `stable`
- `proc checked_sub` signature `proc checked_sub<T>(left: T, right: T) -> Checked<T> {` example `number.checked_sub(...)` stability `stable`
- `proc checked_mul` signature `proc checked_mul<T>(left: T, right: T) -> Checked<T> {` example `number.checked_mul(...)` stability `stable`
- `proc checked_div` signature `proc checked_div<T>(left: T, right: T) -> Checked<T> {` example `number.checked_div(...)` stability `stable`
- `proc checked_rem` signature `proc checked_rem<T>(left: T, right: T) -> Checked<T> {` example `number.checked_rem(...)` stability `stable`
- `proc checked_neg` signature `proc checked_neg<T>(value: T) -> Checked<T> {` example `number.checked_neg(...)` stability `stable`
- `proc checked_shl` signature `proc checked_shl<T>(value: T, amount: u32) -> Checked<T> {` example `number.checked_shl(...)` stability `stable`
- `proc checked_shr` signature `proc checked_shr<T>(value: T, amount: u32) -> Checked<T> {` example `number.checked_shr(...)` stability `stable`
- `proc saturating_add` signature `proc saturating_add<T>(left: T, right: T) -> T {` example `number.saturating_add(...)` stability `stable`
- `proc saturating_sub` signature `proc saturating_sub<T>(left: T, right: T) -> T {` example `number.saturating_sub(...)` stability `stable`
- `proc saturating_mul` signature `proc saturating_mul<T>(left: T, right: T) -> T {` example `number.saturating_mul(...)` stability `stable`
- `proc saturating_div` signature `proc saturating_div<T>(left: T, right: T) -> T {` example `number.saturating_div(...)` stability `stable`
- `proc wrapping_add` signature `proc wrapping_add<T>(left: T, right: T) -> T {` example `number.wrapping_add(...)` stability `stable`
- `proc wrapping_sub` signature `proc wrapping_sub<T>(left: T, right: T) -> T {` example `number.wrapping_sub(...)` stability `stable`
- `proc wrapping_mul` signature `proc wrapping_mul<T>(left: T, right: T) -> T {` example `number.wrapping_mul(...)` stability `stable`
- `proc wrapping_div` signature `proc wrapping_div<T>(left: T, right: T) -> T {` example `number.wrapping_div(...)` stability `stable`
- `proc overflowing_add` signature `proc overflowing_add<T>(left: T, right: T) -> Overflowing<T> {` example `number.overflowing_add(...)` stability `stable`
- `proc overflowing_sub` signature `proc overflowing_sub<T>(left: T, right: T) -> Overflowing<T> {` example `number.overflowing_sub(...)` stability `stable`
- `proc overflowing_mul` signature `proc overflowing_mul<T>(left: T, right: T) -> Overflowing<T> {` example `number.overflowing_mul(...)` stability `stable`
- `proc overflowing_div` signature `proc overflowing_div<T>(left: T, right: T) -> Overflowing<T> {` example `number.overflowing_div(...)` stability `stable`
- `proc abs` signature `proc abs<T>(value: T) -> T {` example `number.abs(...)` stability `stable`
- `proc sign` signature `proc sign<T>(value: T) -> Sign {` example `number.sign(...)` stability `stable`
- `proc signum` signature `proc signum<T>(value: T) -> T {` example `number.signum(...)` stability `stable`
- `proc pow` signature `proc pow<T>(base: T, exponent: u64) -> T {` example `number.pow(...)` stability `stable`
- `proc pow_mod` signature `proc pow_mod<T>(base: T, exponent: T, modulus: T) -> T {` example `number.pow_mod(...)` stability `stable`
- `proc gcd` signature `proc gcd<T>(left: T, right: T) -> T {` example `number.gcd(...)` stability `stable`
- `proc lcm` signature `proc lcm<T>(left: T, right: T) -> T {` example `number.lcm(...)` stability `stable`
- `proc is_even` signature `proc is_even<T>(value: T) -> bool {` example `number.is_even(...)` stability `stable`
- `proc is_odd` signature `proc is_odd<T>(value: T) -> bool {` example `number.is_odd(...)` stability `stable`
- `proc count_ones` signature `proc count_ones<T>(value: T) -> u32 {` example `number.count_ones(...)` stability `stable`
- `proc count_zeros` signature `proc count_zeros<T>(value: T) -> u32 {` example `number.count_zeros(...)` stability `stable`
- `proc leading_zeros` signature `proc leading_zeros<T>(value: T) -> u32 {` example `number.leading_zeros(...)` stability `stable`
- `proc trailing_zeros` signature `proc trailing_zeros<T>(value: T) -> u32 {` example `number.trailing_zeros(...)` stability `stable`
- `proc rotate_left` signature `proc rotate_left<T>(value: T, amount: u32) -> T {` example `number.rotate_left(...)` stability `stable`
- `proc rotate_right` signature `proc rotate_right<T>(value: T, amount: u32) -> T {` example `number.rotate_right(...)` stability `stable`
- `proc reverse_bits` signature `proc reverse_bits<T>(value: T) -> T {` example `number.reverse_bits(...)` stability `stable`
- `proc reverse_bytes` signature `proc reverse_bytes<T>(value: T) -> T {` example `number.reverse_bytes(...)` stability `stable`
- `proc to_little_endian` signature `proc to_little_endian<T>(value: T) -> T {` example `number.to_little_endian(...)` stability `stable`
- `proc to_big_endian` signature `proc to_big_endian<T>(value: T) -> T {` example `number.to_big_endian(...)` stability `stable`
- `proc from_little_endian` signature `proc from_little_endian<T>(value: T) -> T {` example `number.from_little_endian(...)` stability `stable`
- `proc from_big_endian` signature `proc from_big_endian<T>(value: T) -> T {` example `number.from_big_endian(...)` stability `stable`
- `proc endian_convert` signature `proc endian_convert<T>(value: T, from: Endian, to: Endian) -> T {` example `number.endian_convert(...)` stability `stable`

## `src/vitte/stdlib/core/panic.vitl`

Stability: `stable`

- `pick PanicLevel` signature `pick PanicLevel {` example `panic.PanicLevel` stability `stable`
- `form PanicLocation` signature `form PanicLocation {` example `panic.PanicLocation` stability `stable`
- `form PanicFrame` signature `form PanicFrame {` example `panic.PanicFrame` stability `stable`
- `form PanicData` signature `form PanicData {` example `panic.PanicData` stability `stable`
- `form PanicResult` signature `form PanicResult {` example `panic.PanicResult` stability `stable`
- `proc panic_location` signature `proc panic_location(file: string, line: int, column: int) -> PanicLocation {` example `panic.panic_location(...)` stability `stable`
- `proc panic_frame` signature `proc panic_frame(function_name: string, location: PanicLocation) -> PanicFrame {` example `panic.panic_frame(...)` stability `stable`
- `proc panic_data` signature `proc panic_data(level: PanicLevel, message: string, location: PanicLocation, recoverable: bool) -> PanicData {` example `panic.panic_data(...)` stability `stable`
- `proc panic_result` signature `proc panic_result(triggered: bool, recoverable: bool, message: string) -> PanicResult {` example `panic.panic_result(...)` stability `stable`
- `proc info` signature `proc info(message: string) -> PanicResult {` example `panic.info(...)` stability `stable`
- `proc warning` signature `proc warning(message: string) -> PanicResult {` example `panic.warning(...)` stability `stable`
- `proc error` signature `proc error(message: string) -> PanicResult {` example `panic.error(...)` stability `stable`
- `proc panic` signature `proc panic(message: string) -> PanicResult {` example `panic.panic(...)` stability `stable`
- `proc fatal` signature `proc fatal(message: string) -> PanicResult {` example `panic.fatal(...)` stability `stable`
- `proc unreachable` signature `proc unreachable(message: string) -> PanicResult {` example `panic.unreachable(...)` stability `stable`
- `proc todo` signature `proc todo(message: string) -> PanicResult {` example `panic.todo(...)` stability `stable`
- `proc assert` signature `proc assert(condition: bool, message: string) -> PanicResult {` example `panic.assert(...)` stability `stable`
- `proc assert_eq_i64` signature `proc assert_eq_i64(left: i64, right: i64, message: string) -> PanicResult {` example `panic.assert_eq_i64(...)` stability `stable`
- `proc assert_not_eq_i64` signature `proc assert_not_eq_i64(left: i64, right: i64, message: string) -> PanicResult {` example `panic.assert_not_eq_i64(...)` stability `stable`
- `proc assert_string` signature `proc assert_string(left: string, right: string, message: string) -> PanicResult {` example `panic.assert_string(...)` stability `stable`
- `proc panic_triggered` signature `proc panic_triggered(result: PanicResult) -> bool {` example `panic.panic_triggered(...)` stability `stable`
- `proc panic_recoverable` signature `proc panic_recoverable(result: PanicResult) -> bool {` example `panic.panic_recoverable(...)` stability `stable`
- `proc panic_message` signature `proc panic_message(result: PanicResult) -> string {` example `panic.panic_message(...)` stability `stable`
- `proc add_frame` signature `proc add_frame(data: PanicData, frame: PanicFrame) -> PanicData {` example `panic.add_frame(...)` stability `stable`
- `proc build_info` signature `proc build_info(message: string, file: string, line: int, column: int) -> PanicData {` example `panic.build_info(...)` stability `stable`
- `proc build_warning` signature `proc build_warning(message: string, file: string, line: int, column: int) -> PanicData {` example `panic.build_warning(...)` stability `stable`
- `proc build_error` signature `proc build_error(message: string, file: string, line: int, column: int) -> PanicData {` example `panic.build_error(...)` stability `stable`
- `proc build_fatal` signature `proc build_fatal(message: string, file: string, line: int, column: int) -> PanicData {` example `panic.build_fatal(...)` stability `stable`
- `proc panic_level_name` signature `proc panic_level_name(level: PanicLevel) -> string {` example `panic.panic_level_name(...)` stability `stable`
- `proc format_location` signature `proc format_location(location: PanicLocation) -> string {` example `panic.format_location(...)` stability `stable`
- `proc format_frame` signature `proc format_frame(frame: PanicFrame) -> string {` example `panic.format_frame(...)` stability `stable`
- `proc format_stacktrace` signature `proc format_stacktrace(frames: [PanicFrame]) -> string {` example `panic.format_stacktrace(...)` stability `stable`
- `proc format_panic` signature `proc format_panic(data: PanicData) -> string {` example `panic.format_panic(...)` stability `stable`
- `proc panic_selftest` signature `proc panic_selftest() -> bool {` example `panic.panic_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/option.vitl`

Stability: `stable`

- `pick OptionState` signature `pick OptionState {` example `option.OptionState` stability `stable`
- `form Option` signature `form Option<T> {` example `option.Option` stability `stable`
- `form OptionIterator` signature `form OptionIterator<T> {` example `option.OptionIterator` stability `stable`
- `proc some` signature `proc some<T>(value: T) -> Option<T> {` example `option.some(...)` stability `stable`
- `proc none` signature `proc none<T>() -> Option<T> {` example `option.none(...)` stability `stable`
- `proc is_some` signature `proc is_some<T>(option: Option<T>) -> bool {` example `option.is_some(...)` stability `stable`
- `proc is_none` signature `proc is_none<T>(option: Option<T>) -> bool {` example `option.is_none(...)` stability `stable`
- `proc unwrap` signature `proc unwrap<T>(option: Option<T>) -> T {` example `option.unwrap(...)` stability `stable`
- `proc expect` signature `proc expect<T>(option: Option<T>, message: string) -> T {` example `option.expect(...)` stability `stable`
- `proc unwrap_or` signature `proc unwrap_or<T>(option: Option<T>, fallback: T) -> T {` example `option.unwrap_or(...)` stability `stable`
- `proc unwrap_or_else` signature `proc unwrap_or_else<T>(option: Option<T>, fallback: proc() -> T) -> T {` example `option.unwrap_or_else(...)` stability `stable`
- `proc map` signature `proc map<T, U>(option: Option<T>, f: proc(T) -> U) -> Option<U> {` example `option.map(...)` stability `stable`
- `proc map_or` signature `proc map_or<T, U>(option: Option<T>, fallback: U, f: proc(T) -> U) -> U {` example `option.map_or(...)` stability `stable`
- `proc map_or_else` signature `proc map_or_else<T, U>(option: Option<T>, fallback: proc() -> U, f: proc(T) -> U) -> U {` example `option.map_or_else(...)` stability `stable`
- `proc and` signature `proc and<T, U>(option: Option<T>, next: Option<U>) -> Option<U> {` example `option.and(...)` stability `stable`
- `proc and_then` signature `proc and_then<T, U>(option: Option<T>, f: proc(T) -> Option<U>) -> Option<U> {` example `option.and_then(...)` stability `stable`
- `proc or` signature `proc or<T>(option: Option<T>, fallback: Option<T>) -> Option<T> {` example `option.or(...)` stability `stable`
- `proc or_else` signature `proc or_else<T>(option: Option<T>, fallback: proc() -> Option<T>) -> Option<T> {` example `option.or_else(...)` stability `stable`
- `proc xor` signature `proc xor<T>(left: Option<T>, right: Option<T>) -> Option<T> {` example `option.xor(...)` stability `stable`
- `proc filter` signature `proc filter<T>(option: Option<T>, predicate: proc(T) -> bool) -> Option<T> {` example `option.filter(...)` stability `stable`
- `proc take` signature `proc take<T>(slot: ref Option<T>) -> Option<T> {` example `option.take(...)` stability `stable`
- `proc replace` signature `proc replace<T>(slot: ref Option<T>, value: T) -> Option<T> {` example `option.replace(...)` stability `stable`
- `proc insert` signature `proc insert<T>(slot: ref Option<T>, value: T) -> ref T {` example `option.insert(...)` stability `stable`
- `proc get_or_insert` signature `proc get_or_insert<T>(slot: ref Option<T>, value: T) -> ref T {` example `option.get_or_insert(...)` stability `stable`
- `proc get_or_insert_with` signature `proc get_or_insert_with<T>(slot: ref Option<T>, f: proc() -> T) -> ref T {` example `option.get_or_insert_with(...)` stability `stable`
- `proc as_ref` signature `proc as_ref<T>(option: ref Option<T>) -> Option<ref T> {` example `option.as_ref(...)` stability `stable`
- `proc as_mut` signature `proc as_mut<T>(option: ref mut Option<T>) -> Option<ref mut T> {` example `option.as_mut(...)` stability `stable`
- `proc flatten` signature `proc flatten<T>(option: Option<Option<T>>) -> Option<T> {` example `option.flatten(...)` stability `stable`
- `proc transpose` signature `proc transpose<T, E>(option: Option<Result<T, E>>) -> Result<Option<T>, E> {` example `option.transpose(...)` stability `stable`
- `proc option_iter` signature `proc option_iter<T>(option: Option<T>) -> OptionIterator<T> {` example `option.option_iter(...)` stability `stable`
- `proc option_iter_next` signature `proc option_iter_next<T>(iter: ref OptionIterator<T>) -> Option<T> {` example `option.option_iter_next(...)` stability `stable`

## `src/vitte/stdlib/core/primitive.vitl`

Stability: `stable`

- `pick PrimitiveKind` signature `pick PrimitiveKind {` example `primitive.PrimitiveKind` stability `stable`
- `form PrimitiveLayout` signature `form PrimitiveLayout {` example `primitive.PrimitiveLayout` stability `stable`
- `const PRIMITIVE_POINTER_32_SIZE` signature `const PRIMITIVE_POINTER_32_SIZE: u64 = 4` example `primitive.PRIMITIVE_POINTER_32_SIZE` stability `stable`
- `const PRIMITIVE_POINTER_64_SIZE` signature `const PRIMITIVE_POINTER_64_SIZE: u64 = 8` example `primitive.PRIMITIVE_POINTER_64_SIZE` stability `stable`
- `const PRIMITIVE_SLICE_WORDS` signature `const PRIMITIVE_SLICE_WORDS: u64 = 2` example `primitive.PRIMITIVE_SLICE_WORDS` stability `stable`
- `const PRIMITIVE_F16_BACKEND_FEATURE` signature `const PRIMITIVE_F16_BACKEND_FEATURE: string = "float16"` example `primitive.PRIMITIVE_F16_BACKEND_FEATURE` stability `stable`
- `const PRIMITIVE_F128_BACKEND_FEATURE` signature `const PRIMITIVE_F128_BACKEND_FEATURE: string = "float128"` example `primitive.PRIMITIVE_F128_BACKEND_FEATURE` stability `stable`
- `proc primitive_layout` signature `proc primitive_layout(kind: PrimitiveKind, name: string, size_bytes: u64, align_bytes: u64) -> PrimitiveLayout {` example `primitive.primitive_layout(...)` stability `stable`
- `proc primitive_signed_integer` signature `proc primitive_signed_integer(kind: PrimitiveKind, name: string, size_bytes: u64, align_bytes: u64) -> PrimitiveLayout {` example `primitive.primitive_signed_integer(...)` stability `stable`
- `proc primitive_unsigned_integer` signature `proc primitive_unsigned_integer(kind: PrimitiveKind, name: string, size_bytes: u64, align_bytes: u64) -> PrimitiveLayout {` example `primitive.primitive_unsigned_integer(...)` stability `stable`
- `proc primitive_float` signature `proc primitive_float(kind: PrimitiveKind, name: string, size_bytes: u64, align_bytes: u64, backend_optional: bool, feature: string) -> PrimitiveLayout {` example `primitive.primitive_float(...)` stability `stable`
- `proc primitive_pointer_sized` signature `proc primitive_pointer_sized(kind: PrimitiveKind, name: string, pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.primitive_pointer_sized(...)` stability `stable`
- `proc primitive_unsized` signature `proc primitive_unsized(kind: PrimitiveKind, name: string) -> PrimitiveLayout {` example `primitive.primitive_unsized(...)` stability `stable`
- `proc bool_type` signature `proc bool_type() -> PrimitiveLayout {` example `primitive.bool_type(...)` stability `stable`
- `proc byte_type` signature `proc byte_type() -> PrimitiveLayout {` example `primitive.byte_type(...)` stability `stable`
- `proc char_type` signature `proc char_type() -> PrimitiveLayout {` example `primitive.char_type(...)` stability `stable`
- `proc rune_type` signature `proc rune_type() -> PrimitiveLayout {` example `primitive.rune_type(...)` stability `stable`
- `proc int_type` signature `proc int_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.int_type(...)` stability `stable`
- `proc uint_type` signature `proc uint_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.uint_type(...)` stability `stable`
- `proc isize_type` signature `proc isize_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.isize_type(...)` stability `stable`
- `proc usize_type` signature `proc usize_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.usize_type(...)` stability `stable`
- `proc i8_type` signature `proc i8_type() -> PrimitiveLayout {` example `primitive.i8_type(...)` stability `stable`
- `proc i16_type` signature `proc i16_type() -> PrimitiveLayout {` example `primitive.i16_type(...)` stability `stable`
- `proc i32_type` signature `proc i32_type() -> PrimitiveLayout {` example `primitive.i32_type(...)` stability `stable`
- `proc i64_type` signature `proc i64_type() -> PrimitiveLayout {` example `primitive.i64_type(...)` stability `stable`
- `proc i128_type` signature `proc i128_type() -> PrimitiveLayout {` example `primitive.i128_type(...)` stability `stable`
- `proc u8_type` signature `proc u8_type() -> PrimitiveLayout {` example `primitive.u8_type(...)` stability `stable`
- `proc u16_type` signature `proc u16_type() -> PrimitiveLayout {` example `primitive.u16_type(...)` stability `stable`
- `proc u32_type` signature `proc u32_type() -> PrimitiveLayout {` example `primitive.u32_type(...)` stability `stable`
- `proc u64_type` signature `proc u64_type() -> PrimitiveLayout {` example `primitive.u64_type(...)` stability `stable`
- `proc u128_type` signature `proc u128_type() -> PrimitiveLayout {` example `primitive.u128_type(...)` stability `stable`
- `proc f16_type` signature `proc f16_type() -> PrimitiveLayout {` example `primitive.f16_type(...)` stability `stable`
- `proc f32_type` signature `proc f32_type() -> PrimitiveLayout {` example `primitive.f32_type(...)` stability `stable`
- `proc f64_type` signature `proc f64_type() -> PrimitiveLayout {` example `primitive.f64_type(...)` stability `stable`
- `proc f128_type` signature `proc f128_type() -> PrimitiveLayout {` example `primitive.f128_type(...)` stability `stable`
- `proc never_type` signature `proc never_type() -> PrimitiveLayout {` example `primitive.never_type(...)` stability `stable`
- `proc unit_type` signature `proc unit_type() -> PrimitiveLayout {` example `primitive.unit_type(...)` stability `stable`
- `proc tuple_type` signature `proc tuple_type() -> PrimitiveLayout {` example `primitive.tuple_type(...)` stability `stable`
- `proc fixed_array_type` signature `proc fixed_array_type(element_size: u64, element_align: u64, count: u64) -> PrimitiveLayout {` example `primitive.fixed_array_type(...)` stability `stable`
- `proc slice_type` signature `proc slice_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.slice_type(...)` stability `stable`
- `proc pointer_type` signature `proc pointer_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.pointer_type(...)` stability `stable`
- `proc reference_type` signature `proc reference_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.reference_type(...)` stability `stable`
- `proc function_pointer_type` signature `proc function_pointer_type(pointer_width_bytes: u64) -> PrimitiveLayout {` example `primitive.function_pointer_type(...)` stability `stable`
- `proc primitive_layouts` signature `proc primitive_layouts(pointer_width_bytes: u64) -> [PrimitiveLayout] {` example `primitive.primitive_layouts(...)` stability `stable`

## `src/vitte/stdlib/core/range.vitl`

Stability: `stable`

- `pick BoundKind` signature `pick BoundKind {` example `range.BoundKind` stability `stable`
- `pick RangeKind` signature `pick RangeKind {` example `range.RangeKind` stability `stable`
- `form Bound` signature `form Bound<T> {` example `range.Bound` stability `stable`
- `form Range` signature `form Range<T> {` example `range.Range` stability `stable`
- `form RangeCheck` signature `form RangeCheck {` example `range.RangeCheck` stability `stable`
- `form RangeIterator` signature `form RangeIterator<T> {` example `range.RangeIterator` stability `stable`
- `proc included` signature `proc included<T>(value: T) -> Bound<T> {` example `range.included(...)` stability `stable`
- `proc excluded` signature `proc excluded<T>(value: T) -> Bound<T> {` example `range.excluded(...)` stability `stable`
- `proc unbounded` signature `proc unbounded<T>() -> Bound<T> {` example `range.unbounded(...)` stability `stable`
- `proc range_check_ok` signature `proc range_check_ok() -> RangeCheck {` example `range.range_check_ok(...)` stability `stable`
- `proc range_check_error` signature `proc range_check_error(message: string) -> RangeCheck {` example `range.range_check_error(...)` stability `stable`
- `proc detect_zero_step` signature `proc detect_zero_step<T>(step: T) -> RangeCheck {` example `range.detect_zero_step(...)` stability `stable`
- `proc detect_range_overflow` signature `proc detect_range_overflow<T>(start: Bound<T>, end: Bound<T>, step: T) -> RangeCheck {` example `range.detect_range_overflow(...)` stability `stable`
- `proc range_contains_bound` signature `proc range_contains_bound<T>(bound: Bound<T>, value: T, is_start: bool) -> bool {` example `range.range_contains_bound(...)` stability `stable`
- `proc contains` signature `proc contains<T>(range: Range<T>, value: T) -> bool {` example `range.contains(...)` stability `stable`
- `proc range_with_bounds` signature `proc range_with_bounds<T>(start: Bound<T>, end: Bound<T>, step: T, kind: RangeKind) -> Range<T> {` example `range.range_with_bounds(...)` stability `stable`
- `proc exclusive` signature `proc exclusive<T>(start: T, end: T, step: T) -> Range<T> {` example `range.exclusive(...)` stability `stable`
- `proc inclusive` signature `proc inclusive<T>(start: T, end: T, step: T) -> Range<T> {` example `range.inclusive(...)` stability `stable`
- `proc open` signature `proc open<T>(step: T) -> Range<T> {` example `range.open(...)` stability `stable`
- `proc full` signature `proc full<T>() -> Range<T> {` example `range.full(...)` stability `stable`
- `proc from` signature `proc from<T>(start: T, step: T) -> Range<T> {` example `range.from(...)` stability `stable`
- `proc to` signature `proc to<T>(end: T, step: T) -> Range<T> {` example `range.to(...)` stability `stable`
- `proc positive_step` signature `proc positive_step<T>(value: T) -> T {` example `range.positive_step(...)` stability `stable`
- `proc negative_step` signature `proc negative_step<T>(value: T) -> T {` example `range.negative_step(...)` stability `stable`
- `proc iter` signature `proc iter<T>(range: Range<T>) -> RangeIterator<T> {` example `range.iter(...)` stability `stable`
- `proc next` signature `proc next<T>(iter: ref mut RangeIterator<T>) -> Option<T> {` example `range.next(...)` stability `stable`

## `src/vitte/stdlib/core/result.vitl`

Stability: `stable`

- `pick ResultState` signature `pick ResultState {` example `result.ResultState` stability `stable`
- `form Result` signature `form Result<T, E> {` example `result.Result` stability `stable`
- `form ResultIterator` signature `form ResultIterator<T, E> {` example `result.ResultIterator` stability `stable`
- `proc ok` signature `proc ok<T, E>(value: T) -> Result<T, E> {` example `result.ok(...)` stability `stable`
- `proc err` signature `proc err<T, E>(error: E) -> Result<T, E> {` example `result.err(...)` stability `stable`
- `proc is_ok` signature `proc is_ok<T, E>(result: Result<T, E>) -> bool {` example `result.is_ok(...)` stability `stable`
- `proc is_err` signature `proc is_err<T, E>(result: Result<T, E>) -> bool {` example `result.is_err(...)` stability `stable`
- `proc unwrap` signature `proc unwrap<T, E>(result: Result<T, E>) -> T {` example `result.unwrap(...)` stability `stable`
- `proc unwrap_err` signature `proc unwrap_err<T, E>(result: Result<T, E>) -> E {` example `result.unwrap_err(...)` stability `stable`
- `proc expect` signature `proc expect<T, E>(result: Result<T, E>, message: string) -> T {` example `result.expect(...)` stability `stable`
- `proc expect_err` signature `proc expect_err<T, E>(result: Result<T, E>, message: string) -> E {` example `result.expect_err(...)` stability `stable`
- `proc map` signature `proc map<T, U, E>(result: Result<T, E>, f: proc(T) -> U) -> Result<U, E> {` example `result.map(...)` stability `stable`
- `proc map_err` signature `proc map_err<T, E, F>(result: Result<T, E>, f: proc(E) -> F) -> Result<T, F> {` example `result.map_err(...)` stability `stable`
- `proc map_or` signature `proc map_or<T, U, E>(result: Result<T, E>, fallback: U, f: proc(T) -> U) -> U {` example `result.map_or(...)` stability `stable`
- `proc map_or_else` signature `proc map_or_else<T, U, E>(result: Result<T, E>, fallback: proc(E) -> U, f: proc(T) -> U) -> U {` example `result.map_or_else(...)` stability `stable`
- `proc and` signature `proc and<T, U, E>(result: Result<T, E>, next: Result<U, E>) -> Result<U, E> {` example `result.and(...)` stability `stable`
- `proc and_then` signature `proc and_then<T, U, E>(result: Result<T, E>, f: proc(T) -> Result<U, E>) -> Result<U, E> {` example `result.and_then(...)` stability `stable`
- `proc or` signature `proc or<T, E, F>(result: Result<T, E>, fallback: Result<T, F>) -> Result<T, F> {` example `result.or(...)` stability `stable`
- `proc or_else` signature `proc or_else<T, E, F>(result: Result<T, E>, fallback: proc(E) -> Result<T, F>) -> Result<T, F> {` example `result.or_else(...)` stability `stable`
- `proc unwrap_or` signature `proc unwrap_or<T, E>(result: Result<T, E>, fallback: T) -> T {` example `result.unwrap_or(...)` stability `stable`
- `proc unwrap_or_else` signature `proc unwrap_or_else<T, E>(result: Result<T, E>, fallback: proc(E) -> T) -> T {` example `result.unwrap_or_else(...)` stability `stable`
- `proc as_ref` signature `proc as_ref<T, E>(result: ref Result<T, E>) -> Result<ref T, ref E> {` example `result.as_ref(...)` stability `stable`
- `proc as_mut` signature `proc as_mut<T, E>(result: ref mut Result<T, E>) -> Result<ref mut T, ref mut E> {` example `result.as_mut(...)` stability `stable`
- `proc flatten` signature `proc flatten<T, E>(result: Result<Result<T, E>, E>) -> Result<T, E> {` example `result.flatten(...)` stability `stable`
- `proc transpose` signature `proc transpose<T, E>(result: Result<Option<T>, E>) -> Option<Result<T, E>> {` example `result.transpose(...)` stability `stable`
- `proc result_iter` signature `proc result_iter<T, E>(result: Result<T, E>) -> ResultIterator<T, E> {` example `result.result_iter(...)` stability `stable`
- `proc result_iter_next` signature `proc result_iter_next<T, E>(iter: ref ResultIterator<T, E>) -> Option<T> {` example `result.result_iter_next(...)` stability `stable`
- `proc propagate` signature `proc propagate<T, E>(result: Result<T, E>) -> T {` example `result.propagate(...)` stability `stable`
- `proc try_result` signature `proc try_result<T, E>(result: Result<T, E>) -> T {` example `result.try_result(...)` stability `stable`

## `src/vitte/stdlib/core/scope.vitl`

Stability: `stable`

- `pick ScopeExitReason` signature `pick ScopeExitReason {` example `scope.ScopeExitReason` stability `stable`
- `form ScopeGuard` signature `form ScopeGuard<T> {` example `scope.ScopeGuard` stability `stable`
- `form DeferAction` signature `form DeferAction {` example `scope.DeferAction` stability `stable`
- `proc scope_guard` signature `proc scope_guard<T>(value: T, action: proc(ref mut T) -> unit) -> ScopeGuard<T> {` example `scope.scope_guard(...)` stability `stable`
- `proc dismiss` signature `proc dismiss<T>(guard: ref mut ScopeGuard<T>) {` example `scope.dismiss(...)` stability `stable`
- `proc run_scope_guard` signature `proc run_scope_guard<T>(guard: ref mut ScopeGuard<T>) {` example `scope.run_scope_guard(...)` stability `stable`
- `proc defer` signature `proc defer(name: string, run: proc() -> unit) -> DeferAction {` example `scope.defer(...)` stability `stable`
- `proc run_defer` signature `proc run_defer(action: ref mut DeferAction) {` example `scope.run_defer(...)` stability `stable`
- `proc defer_runs_for_exit` signature `proc defer_runs_for_exit(reason: ScopeExitReason) -> bool {` example `scope.defer_runs_for_exit(...)` stability `stable`
- `proc defer_panic_behavior` signature `proc defer_panic_behavior() -> string {` example `scope.defer_panic_behavior(...)` stability `stable`
- `proc scope_exit_guarantee` signature `proc scope_exit_guarantee(reason: ScopeExitReason, action: DeferAction) -> bool {` example `scope.scope_exit_guarantee(...)` stability `stable`

## `src/vitte/stdlib/core/slice.vitl`

Stability: `stable`

- `form Slice` signature `form Slice<T> {` example `slice.Slice` stability `stable`
- `form SliceIndex` signature `form SliceIndex {` example `slice.SliceIndex` stability `stable`
- `form SliceChunks` signature `form SliceChunks<T> {` example `slice.SliceChunks` stability `stable`
- `form SliceWindows` signature `form SliceWindows<T> {` example `slice.SliceWindows` stability `stable`
- `proc len` signature `proc len<T>(slice: Slice<T>) -> usize { give slice.length; }` example `slice.len(...)` stability `stable`
- `proc is_empty` signature `proc is_empty<T>(slice: Slice<T>) -> bool { give slice.length == 0; }` example `slice.is_empty(...)` stability `stable`
- `proc get` signature `proc get<T>(slice: Slice<T>, index: usize) -> Option<ref T> {` example `slice.get(...)` stability `stable`
- `proc get_mut` signature `proc get_mut<T>(slice: ref mut Slice<T>, index: usize) -> Option<ref mut T> { give compiler_slice_get_mut<T>(slice, index); }` example `slice.get_mut(...)` stability `stable`
- `proc first` signature `proc first<T>(slice: Slice<T>) -> Option<ref T> { give get<T>(slice, 0); }` example `slice.first(...)` stability `stable`
- `proc last` signature `proc last<T>(slice: Slice<T>) -> Option<ref T> {` example `slice.last(...)` stability `stable`
- `proc split_at` signature `proc split_at<T>(slice: Slice<T>, index: usize) -> (Slice<T>, Slice<T>) {` example `slice.split_at(...)` stability `stable`
- `proc subslice` signature `proc subslice<T>(slice: Slice<T>, start: usize, end: usize) -> Option<Slice<T>> {` example `slice.subslice(...)` stability `stable`
- `proc iter` signature `proc iter<T>(slice: Slice<T>) -> Iterator<ref T> { give compiler_slice_iter<T>(slice); }` example `slice.iter(...)` stability `stable`
- `proc chunks` signature `proc chunks<T>(slice: Slice<T>, size: usize) -> SliceChunks<T> {` example `slice.chunks(...)` stability `stable`
- `proc windows` signature `proc windows<T>(slice: Slice<T>, size: usize) -> SliceWindows<T> {` example `slice.windows(...)` stability `stable`
- `proc binary_search` signature `proc binary_search<T>(slice: Slice<T>, needle: T, compare: proc(T, T) -> int) -> Option<usize> {` example `slice.binary_search(...)` stability `stable`
- `proc sort_unstable` signature `proc sort_unstable<T>(slice: ref mut Slice<T>, compare: proc(T, T) -> int) {` example `slice.sort_unstable(...)` stability `stable`
- `proc sort_stable` signature `proc sort_stable<T>(slice: ref mut Slice<T>, compare: proc(T, T) -> int) {` example `slice.sort_stable(...)` stability `stable`
- `proc fill` signature `proc fill<T>(slice: ref mut Slice<T>, value: T) { compiler_slice_fill<T>(slice, value); }` example `slice.fill(...)` stability `stable`
- `proc copy_from` signature `proc copy_from<T>(dst: ref mut Slice<T>, src: Slice<T>) { compiler_slice_copy_from<T>(dst, src); }` example `slice.copy_from(...)` stability `stable`

## `src/vitte/stdlib/core/string.vitl`

Stability: `stable`

- `form Utf8View` signature `form Utf8View {` example `string.Utf8View` stability `stable`
- `form CharIndex` signature `form CharIndex {` example `string.CharIndex` stability `stable`
- `form SplitOnce` signature `form SplitOnce {` example `string.SplitOnce` stability `stable`
- `pick StringOrdering` signature `pick StringOrdering {` example `string.StringOrdering` stability `stable`
- `form Utf8Decode` signature `form Utf8Decode {` example `string.Utf8Decode` stability `stable`
- `form SplitIterator` signature `form SplitIterator {` example `string.SplitIterator` stability `stable`
- `proc utf8_view` signature `proc utf8_view(text: string) -> Utf8View {` example `string.utf8_view(...)` stability `stable`
- `proc byte_length` signature `proc byte_length(text: string) -> usize {` example `string.byte_length(...)` stability `stable`
- `proc char_length` signature `proc char_length(text: string) -> usize {` example `string.char_length(...)` stability `stable`
- `proc bytes` signature `proc bytes(view: Utf8View) -> Iterator<byte> {` example `string.bytes(...)` stability `stable`
- `proc chars` signature `proc chars(view: Utf8View) -> Iterator<rune> {` example `string.chars(...)` stability `stable`
- `proc char_indices` signature `proc char_indices(view: Utf8View) -> Iterator<CharIndex> {` example `string.char_indices(...)` stability `stable`
- `proc lines` signature `proc lines(view: Utf8View) -> Iterator<Utf8View> {` example `string.lines(...)` stability `stable`
- `proc split` signature `proc split(view: Utf8View, separator: string) -> Iterator<Utf8View> {` example `string.split(...)` stability `stable`
- `proc split_iter` signature `proc split_iter(view: Utf8View, separator: string) -> SplitIterator {` example `string.split_iter(...)` stability `stable`
- `proc split_once` signature `proc split_once(view: Utf8View, separator: string) -> SplitOnce {` example `string.split_once(...)` stability `stable`
- `proc split_whitespace` signature `proc split_whitespace(view: Utf8View) -> Iterator<Utf8View> {` example `string.split_whitespace(...)` stability `stable`
- `proc trim` signature `proc trim(view: Utf8View) -> Utf8View {` example `string.trim(...)` stability `stable`
- `proc trim_start` signature `proc trim_start(view: Utf8View) -> Utf8View {` example `string.trim_start(...)` stability `stable`
- `proc trim_end` signature `proc trim_end(view: Utf8View) -> Utf8View {` example `string.trim_end(...)` stability `stable`
- `proc starts_with` signature `proc starts_with(view: Utf8View, prefix: string) -> bool {` example `string.starts_with(...)` stability `stable`
- `proc ends_with` signature `proc ends_with(view: Utf8View, suffix: string) -> bool {` example `string.ends_with(...)` stability `stable`
- `proc contains` signature `proc contains(view: Utf8View, needle: string) -> bool {` example `string.contains(...)` stability `stable`
- `proc find` signature `proc find(view: Utf8View, needle: string) -> Option<usize> {` example `string.find(...)` stability `stable`
- `proc rfind` signature `proc rfind(view: Utf8View, needle: string) -> Option<usize> {` example `string.rfind(...)` stability `stable`
- `proc compare` signature `proc compare(left: Utf8View, right: Utf8View) -> StringOrdering {` example `string.compare(...)` stability `stable`
- `proc equals` signature `proc equals(left: Utf8View, right: Utf8View) -> bool {` example `string.equals(...)` stability `stable`
- `proc validate_utf8` signature `proc validate_utf8(bytes_value: [byte]) -> bool {` example `string.validate_utf8(...)` stability `stable`
- `proc decode_char` signature `proc decode_char(bytes_value: [byte], offset: usize) -> Utf8Decode {` example `string.decode_char(...)` stability `stable`
- `proc encode_utf8` signature `proc encode_utf8(value: rune) -> [byte] {` example `string.encode_utf8(...)` stability `stable`
- `proc is_char_boundary` signature `proc is_char_boundary(view: Utf8View, byte_index: usize) -> bool {` example `string.is_char_boundary(...)` stability `stable`
- `proc checked_byte_index` signature `proc checked_byte_index(view: Utf8View, byte_index: usize) -> Option<byte> {` example `string.checked_byte_index(...)` stability `stable`
- `proc reject_mid_char_index` signature `proc reject_mid_char_index(view: Utf8View, byte_index: usize) {` example `string.reject_mid_char_index(...)` stability `stable`

## `src/vitte/stdlib/core/strings.vitl`

Stability: `stable`

- `proc strlen` signature `proc strlen(text_value: string) -> int {` example `strings.strlen(...)` stability `stable`
- `proc strcmp` signature `proc strcmp(a: string, b: string) -> int {` example `strings.strcmp(...)` stability `stable`
- `proc strcasecmp` signature `proc strcasecmp(a: string, b: string) -> int {` example `strings.strcasecmp(...)` stability `stable`
- `proc streq` signature `proc streq(a: string, b: string) -> bool {` example `strings.streq(...)` stability `stable`
- `proc strne` signature `proc strne(a: string, b: string) -> bool {` example `strings.strne(...)` stability `stable`
- `proc strstr` signature `proc strstr(haystack: string, needle: string) -> int {` example `strings.strstr(...)` stability `stable`
- `proc strchr` signature `proc strchr(text_value: string, ch: int) -> int {` example `strings.strchr(...)` stability `stable`
- `proc strrchr` signature `proc strrchr(text_value: string, ch: int) -> int {` example `strings.strrchr(...)` stability `stable`
- `proc chrcount` signature `proc chrcount(text_value: string, ch: int) -> int {` example `strings.chrcount(...)` stability `stable`
- `proc strbegin` signature `proc strbegin(text_value: string, prefix: string) -> bool {` example `strings.strbegin(...)` stability `stable`
- `proc strend` signature `proc strend(text_value: string, suffix: string) -> bool {` example `strings.strend(...)` stability `stable`
- `proc strreplace_first` signature `proc strreplace_first(text_value: string, old: string, new: string) -> string {` example `strings.strreplace_first(...)` stability `stable`
- `proc strupcase` signature `proc strupcase(text_value: string) -> string {` example `strings.strupcase(...)` stability `stable`
- `proc strdowncase` signature `proc strdowncase(text_value: string) -> string {` example `strings.strdowncase(...)` stability `stable`
- `proc strlstrip` signature `proc strlstrip(text_value: string) -> string {` example `strings.strlstrip(...)` stability `stable`
- `proc strrstrip` signature `proc strrstrip(text_value: string) -> string {` example `strings.strrstrip(...)` stability `stable`
- `proc strstrip` signature `proc strstrip(text_value: string) -> string {` example `strings.strstrip(...)` stability `stable`
- `proc strrepeat` signature `proc strrepeat(text_value: string, count: int) -> string {` example `strings.strrepeat(...)` stability `stable`
- `proc strreverse` signature `proc strreverse(text_value: string) -> string {` example `strings.strreverse(...)` stability `stable`

## `src/vitte/stdlib/core/types.vitl`

Stability: `stable`

- `pick TypeKind` signature `pick TypeKind {` example `types.TypeKind` stability `stable`
- `form TypeField` signature `form TypeField {` example `types.TypeField` stability `stable`
- `form TypeParameter` signature `form TypeParameter {` example `types.TypeParameter` stability `stable`
- `form TypeInfo` signature `form TypeInfo {` example `types.TypeInfo` stability `stable`
- `form TypeCheckResult` signature `form TypeCheckResult {` example `types.TypeCheckResult` stability `stable`
- `proc type_field` signature `proc type_field(name: string, field_type: string, offset: u64, size: u64) -> TypeField {` example `types.type_field(...)` stability `stable`
- `proc type_parameter` signature `proc type_parameter(name: string, constraint: string) -> TypeParameter {` example `types.type_parameter(...)` stability `stable`
- `proc type_info` signature `proc type_info(kind: TypeKind, name: string, size: u64, align: u64) -> TypeInfo {` example `types.type_info(...)` stability `stable`
- `proc type_check_result` signature `proc type_check_result(valid: bool, expected: string, actual: string) -> TypeCheckResult {` example `types.type_check_result(...)` stability `stable`
- `proc void_type` signature `proc void_type() -> TypeInfo {` example `types.void_type(...)` stability `stable`
- `proc bool_type` signature `proc bool_type() -> TypeInfo {` example `types.bool_type(...)` stability `stable`
- `proc char_type` signature `proc char_type() -> TypeInfo {` example `types.char_type(...)` stability `stable`
- `proc string_type` signature `proc string_type() -> TypeInfo {` example `types.string_type(...)` stability `stable`
- `proc i32_type` signature `proc i32_type() -> TypeInfo {` example `types.i32_type(...)` stability `stable`
- `proc i64_type` signature `proc i64_type() -> TypeInfo {` example `types.i64_type(...)` stability `stable`
- `proc u32_type` signature `proc u32_type() -> TypeInfo {` example `types.u32_type(...)` stability `stable`
- `proc u64_type` signature `proc u64_type() -> TypeInfo {` example `types.u64_type(...)` stability `stable`
- `proc f32_type` signature `proc f32_type() -> TypeInfo {` example `types.f32_type(...)` stability `stable`
- `proc f64_type` signature `proc f64_type() -> TypeInfo {` example `types.f64_type(...)` stability `stable`
- `proc pointer_type` signature `proc pointer_type(name: string) -> TypeInfo {` example `types.pointer_type(...)` stability `stable`
- `proc array_type` signature `proc array_type(name: string, size: u64) -> TypeInfo {` example `types.array_type(...)` stability `stable`
- `proc function_type` signature `proc function_type(name: string) -> TypeInfo {` example `types.function_type(...)` stability `stable`
- `proc struct_type` signature `proc struct_type(name: string, fields: [TypeField]) -> TypeInfo {` example `types.struct_type(...)` stability `stable`
- `proc enum_type` signature `proc enum_type(name: string) -> TypeInfo {` example `types.enum_type(...)` stability `stable`
- `proc generic_type` signature `proc generic_type(name: string, parameters: [TypeParameter]) -> TypeInfo {` example `types.generic_type(...)` stability `stable`
- `proc add_field` signature `proc add_field(info: TypeInfo, field: TypeField) -> TypeInfo {` example `types.add_field(...)` stability `stable`
- `proc add_parameter` signature `proc add_parameter(info: TypeInfo, parameter: TypeParameter) -> TypeInfo {` example `types.add_parameter(...)` stability `stable`
- `proc type_name` signature `proc type_name(info: TypeInfo) -> string {` example `types.type_name(...)` stability `stable`
- `proc type_size` signature `proc type_size(info: TypeInfo) -> u64 {` example `types.type_size(...)` stability `stable`
- `proc type_align` signature `proc type_align(info: TypeInfo) -> u64 {` example `types.type_align(...)` stability `stable`
- `proc field_count` signature `proc field_count(info: TypeInfo) -> int {` example `types.field_count(...)` stability `stable`
- `proc parameter_count` signature `proc parameter_count(info: TypeInfo) -> int {` example `types.parameter_count(...)` stability `stable`
- `proc signed_type` signature `proc signed_type(info: TypeInfo) -> bool {` example `types.signed_type(...)` stability `stable`
- `proc same_type` signature `proc same_type(left: TypeInfo, right: TypeInfo) -> bool {` example `types.same_type(...)` stability `stable`
- `proc check_type` signature `proc check_type(expected: TypeInfo, actual: TypeInfo) -> TypeCheckResult {` example `types.check_type(...)` stability `stable`
- `proc type_kind_name` signature `proc type_kind_name(kind: TypeKind) -> string {` example `types.type_kind_name(...)` stability `stable`
- `proc format_type` signature `proc format_type(info: TypeInfo) -> string {` example `types.format_type(...)` stability `stable`
- `proc types_selftest` signature `proc types_selftest() -> bool {` example `types.types_selftest(...)` stability `stable`

## `src/vitte/stdlib/core/unicode.vitl`

Stability: `stable`

- `const UNICODE_VERSION` signature `const UNICODE_VERSION: string = "15.1.0"` example `unicode.UNICODE_VERSION` stability `stable`
- `const UNICODE_TABLES_GENERATED` signature `const UNICODE_TABLES_GENERATED: bool = true` example `unicode.UNICODE_TABLES_GENERATED` stability `stable`
- `const UNICODE_TABLE_GENERATOR` signature `const UNICODE_TABLE_GENERATOR: string = "src/vitte/stdlib/generated/unicode_tables"` example `unicode.UNICODE_TABLE_GENERATOR` stability `stable`
- `const UNICODE_UCD_CHECKSUM` signature `const UNICODE_UCD_CHECKSUM: string = "sha256:local-ucd-contract"` example `unicode.UNICODE_UCD_CHECKSUM` stability `stable`
- `pick NormalizationForm` signature `pick NormalizationForm {` example `unicode.NormalizationForm` stability `stable`
- `pick UnicodeCategory` signature `pick UnicodeCategory {` example `unicode.UnicodeCategory` stability `stable`
- `form UnicodeProperties` signature `form UnicodeProperties {` example `unicode.UnicodeProperties` stability `stable`
- `proc unicode_version` signature `proc unicode_version() -> string {` example `unicode.unicode_version(...)` stability `stable`
- `proc unicode_tables_generated` signature `proc unicode_tables_generated() -> bool {` example `unicode.unicode_tables_generated(...)` stability `stable`
- `proc unicode_table_generator` signature `proc unicode_table_generator() -> string {` example `unicode.unicode_table_generator(...)` stability `stable`
- `proc general_category` signature `proc general_category(value: rune) -> UnicodeCategory {` example `unicode.general_category(...)` stability `stable`
- `proc properties` signature `proc properties(value: rune) -> UnicodeProperties {` example `unicode.properties(...)` stability `stable`
- `proc is_alphabetic` signature `proc is_alphabetic(value: rune) -> bool {` example `unicode.is_alphabetic(...)` stability `stable`
- `proc is_numeric` signature `proc is_numeric(value: rune) -> bool {` example `unicode.is_numeric(...)` stability `stable`
- `proc is_alphanumeric` signature `proc is_alphanumeric(value: rune) -> bool {` example `unicode.is_alphanumeric(...)` stability `stable`
- `proc is_whitespace` signature `proc is_whitespace(value: rune) -> bool {` example `unicode.is_whitespace(...)` stability `stable`
- `proc is_uppercase` signature `proc is_uppercase(value: rune) -> bool {` example `unicode.is_uppercase(...)` stability `stable`
- `proc is_lowercase` signature `proc is_lowercase(value: rune) -> bool {` example `unicode.is_lowercase(...)` stability `stable`
- `proc to_uppercase` signature `proc to_uppercase(value: rune) -> Iterator<rune> {` example `unicode.to_uppercase(...)` stability `stable`
- `proc to_lowercase` signature `proc to_lowercase(value: rune) -> Iterator<rune> {` example `unicode.to_lowercase(...)` stability `stable`
- `proc to_titlecase` signature `proc to_titlecase(value: rune) -> Iterator<rune> {` example `unicode.to_titlecase(...)` stability `stable`
- `proc case_fold` signature `proc case_fold(value: rune) -> Iterator<rune> {` example `unicode.case_fold(...)` stability `stable`
- `proc normalize` signature `proc normalize(text: Utf8View, form: NormalizationForm) -> String {` example `unicode.normalize(...)` stability `stable`
- `proc normalize_nfc` signature `proc normalize_nfc(text: Utf8View) -> String {` example `unicode.normalize_nfc(...)` stability `stable`
- `proc normalize_nfd` signature `proc normalize_nfd(text: Utf8View) -> String {` example `unicode.normalize_nfd(...)` stability `stable`
- `proc normalize_nfkc` signature `proc normalize_nfkc(text: Utf8View) -> String {` example `unicode.normalize_nfkc(...)` stability `stable`
- `proc normalize_nfkd` signature `proc normalize_nfkd(text: Utf8View) -> String {` example `unicode.normalize_nfkd(...)` stability `stable`
- `proc case_fold_string` signature `proc case_fold_string(text: Utf8View) -> String {` example `unicode.case_fold_string(...)` stability `stable`

## `src/vitte/stdlib/core/utils.vitl`

Stability: `stable`

- `form Range` signature `form Range {` example `utils.Range` stability `stable`
- `form Pair` signature `form Pair[T, U] {` example `utils.Pair` stability `stable`
- `form ClampResult` signature `form ClampResult {` example `utils.ClampResult` stability `stable`
- `proc range` signature `proc range(start: int, end: int) -> Range {` example `utils.range(...)` stability `stable`
- `proc pair` signature `proc pair[T, U](first: T, second: U) -> Pair[T, U] {` example `utils.pair(...)` stability `stable`
- `proc clamp_result` signature `proc clamp_result(value: int, changed: bool) -> ClampResult {` example `utils.clamp_result(...)` stability `stable`
- `proc min_i64` signature `proc min_i64(left: i64, right: i64) -> i64 {` example `utils.min_i64(...)` stability `stable`
- `proc max_i64` signature `proc max_i64(left: i64, right: i64) -> i64 {` example `utils.max_i64(...)` stability `stable`
- `proc abs_i64` signature `proc abs_i64(value: i64) -> i64 {` example `utils.abs_i64(...)` stability `stable`
- `proc clamp` signature `proc clamp(value: int, min: int, max: int) -> ClampResult {` example `utils.clamp(...)` stability `stable`
- `proc in_range` signature `proc in_range(value: int, min: int, max: int) -> bool {` example `utils.in_range(...)` stability `stable`
- `proc swap_i64` signature `proc swap_i64(left: i64, right: i64) -> Pair[i64, i64] {` example `utils.swap_i64(...)` stability `stable`
- `proc repeat_string` signature `proc repeat_string(text: string, count: int) -> string {` example `utils.repeat_string(...)` stability `stable`
- `proc join_strings` signature `proc join_strings(values: [string], separator: string) -> string {` example `utils.join_strings(...)` stability `stable`
- `proc starts_with` signature `proc starts_with(text: string, prefix: string) -> bool {` example `utils.starts_with(...)` stability `stable`
- `proc ends_with` signature `proc ends_with(text: string, suffix: string) -> bool {` example `utils.ends_with(...)` stability `stable`
- `proc contains` signature `proc contains(text: string, pattern: string) -> bool {` example `utils.contains(...)` stability `stable`
- `proc reverse_string` signature `proc reverse_string(text: string) -> string {` example `utils.reverse_string(...)` stability `stable`
- `proc split_lines` signature `proc split_lines(text: string) -> [string] {` example `utils.split_lines(...)` stability `stable`
- `proc trim_spaces` signature `proc trim_spaces(text: string) -> string {` example `utils.trim_spaces(...)` stability `stable`
- `proc bool_to_string` signature `proc bool_to_string(value: bool) -> string {` example `utils.bool_to_string(...)` stability `stable`
- `proc parse_bool` signature `proc parse_bool(text: string) -> bool {` example `utils.parse_bool(...)` stability `stable`
- `proc count_char` signature `proc count_char(text: string, target: string) -> int {` example `utils.count_char(...)` stability `stable`
- `proc unique_strings` signature `proc unique_strings(values: [string]) -> [string] {` example `utils.unique_strings(...)` stability `stable`
- `proc utils_selftest` signature `proc utils_selftest() -> bool {` example `utils.utils_selftest(...)` stability `stable`

## `src/vitte/stdlib/alloc/arc.vitl`

Stability: `stable`

- `form Arc` signature `form Arc<T> { ptr: ref T }` example `arc.Arc` stability `stable`
- `form ArcWeak` signature `form ArcWeak<T> { ptr: ref T }` example `arc.ArcWeak` stability `stable`
- `proc arc_new` signature `proc arc_new<T>(value: T) -> Arc<T> { give compiler_arc_new<T>(value); }` example `arc.arc_new(...)` stability `stable`
- `proc arc_clone` signature `proc arc_clone<T>(value: Arc<T>) -> Arc<T> { give compiler_arc_clone<T>(value); }` example `arc.arc_clone(...)` stability `stable`
- `proc arc_strong_count` signature `proc arc_strong_count<T>(value: Arc<T>) -> usize { give compiler_arc_strong_count<T>(value); }` example `arc.arc_strong_count(...)` stability `stable`
- `proc arc_weak_count` signature `proc arc_weak_count<T>(value: Arc<T>) -> usize { give compiler_arc_weak_count<T>(value); }` example `arc.arc_weak_count(...)` stability `stable`
- `proc arc_downgrade` signature `proc arc_downgrade<T>(value: Arc<T>) -> ArcWeak<T> { give compiler_arc_downgrade<T>(value); }` example `arc.arc_downgrade(...)` stability `stable`
- `proc arc_weak_upgrade` signature `proc arc_weak_upgrade<T>(value: ArcWeak<T>) -> Option<Arc<T>> { give compiler_arc_weak_upgrade<T>(value); }` example `arc.arc_weak_upgrade(...)` stability `stable`

## `src/vitte/stdlib/alloc/box.vitl`

Stability: `stable`

- `form Box` signature `form Box<T> {` example `box.Box` stability `stable`
- `proc box_new` signature `proc box_new<T>(value: T) -> Box<T> { give compiler_box_new<T>(value); }` example `box.box_new(...)` stability `stable`
- `proc box_into_inner` signature `proc box_into_inner<T>(boxed: Box<T>) -> T { give compiler_box_into_inner<T>(boxed); }` example `box.box_into_inner(...)` stability `stable`
- `proc box_as_ref` signature `proc box_as_ref<T>(boxed: ref Box<T>) -> ref T { give compiler_box_as_ref<T>(boxed); }` example `box.box_as_ref(...)` stability `stable`
- `proc box_as_mut` signature `proc box_as_mut<T>(boxed: ref mut Box<T>) -> ref mut T { give compiler_box_as_mut<T>(boxed); }` example `box.box_as_mut(...)` stability `stable`
- `proc box_drop` signature `proc box_drop<T>(boxed: Box<T>) { compiler_box_drop<T>(boxed); }` example `box.box_drop(...)` stability `stable`

## `src/vitte/stdlib/alloc/btree.vitl`

Stability: `stable`

- `proc btree_map_new` signature `proc btree_map_new<K, V>() -> BTreeMap<K, V> { give alloc_collections.btreemap_new<K, V>(); }` example `btree.btree_map_new(...)` stability `stable`
- `proc btree_map_insert` signature `proc btree_map_insert<K, V>(map: ref mut BTreeMap<K, V>, key: K, value: V) -> Option<V> { give alloc_collections.btreemap_insert<K, V>(map, key, value); }` example `btree.btree_map_insert(...)` stability `stable`
- `proc btree_map_get` signature `proc btree_map_get<K, V>(map: ref BTreeMap<K, V>, key: K) -> Option<ref V> { give alloc_collections.btreemap_get<K, V>(map, key); }` example `btree.btree_map_get(...)` stability `stable`
- `proc btree_map_remove` signature `proc btree_map_remove<K, V>(map: ref mut BTreeMap<K, V>, key: K) -> Option<V> { give alloc_collections.btreemap_remove<K, V>(map, key); }` example `btree.btree_map_remove(...)` stability `stable`
- `proc btree_set_new` signature `proc btree_set_new<T>() -> BTreeSet<T> { give alloc_collections.btreeset_new<T>(); }` example `btree.btree_set_new(...)` stability `stable`

## `src/vitte/stdlib/alloc/deque.vitl`

Stability: `stable`

- `proc deque_new` signature `proc deque_new<T>() -> Deque<T> { give alloc_collections.deque_new<T>(); }` example `deque.deque_new(...)` stability `stable`
- `proc deque_push_back` signature `proc deque_push_back<T>(deque: ref mut Deque<T>, value: T) { alloc_collections.deque_push_back<T>(deque, value); }` example `deque.deque_push_back(...)` stability `stable`
- `proc deque_push_front` signature `proc deque_push_front<T>(deque: ref mut Deque<T>, value: T) { alloc_collections.deque_push_front<T>(deque, value); }` example `deque.deque_push_front(...)` stability `stable`
- `proc deque_pop_back` signature `proc deque_pop_back<T>(deque: ref mut Deque<T>) -> Option<T> { give alloc_collections.deque_pop_back<T>(deque); }` example `deque.deque_pop_back(...)` stability `stable`
- `proc deque_pop_front` signature `proc deque_pop_front<T>(deque: ref mut Deque<T>) -> Option<T> { give alloc_collections.deque_pop_front<T>(deque); }` example `deque.deque_pop_front(...)` stability `stable`
- `proc deque_len` signature `proc deque_len<T>(deque: Deque<T>) -> usize { give alloc_collections.deque_len<T>(deque); }` example `deque.deque_len(...)` stability `stable`

## `src/vitte/stdlib/alloc/smallvec.vitl`

Stability: `stable`

- `proc smallvec_new` signature `proc smallvec_new<T, N>() -> SmallVec<T, N> { give alloc_collections.smallvec_new<T, N>(); }` example `smallvec.smallvec_new(...)` stability `stable`
- `proc smallvec_len` signature `proc smallvec_len<T, N>(value: SmallVec<T, N>) -> usize { give alloc_collections.smallvec_len<T, N>(value); }` example `smallvec.smallvec_len(...)` stability `stable`
- `proc smallvec_push` signature `proc smallvec_push<T, N>(value: ref mut SmallVec<T, N>, item: T) { alloc_collections.smallvec_push<T, N>(value, item); }` example `smallvec.smallvec_push(...)` stability `stable`
- `proc smallvec_pop` signature `proc smallvec_pop<T, N>(value: ref mut SmallVec<T, N>) -> Option<T> { give alloc_collections.smallvec_pop<T, N>(value); }` example `smallvec.smallvec_pop(...)` stability `stable`

## `src/vitte/stdlib/alloc/collections.vitl`

Stability: `stable`

- `form HashMap` signature `form HashMap<K, V> {` example `collections.HashMap` stability `stable`
- `form HashSet` signature `form HashSet<T> {` example `collections.HashSet` stability `stable`
- `form BTreeMap` signature `form BTreeMap<K, V> {` example `collections.BTreeMap` stability `stable`
- `form BTreeSet` signature `form BTreeSet<T> {` example `collections.BTreeSet` stability `stable`
- `form Deque` signature `form Deque<T> {` example `collections.Deque` stability `stable`
- `form SmallVec` signature `form SmallVec<T, N> {` example `collections.SmallVec` stability `stable`
- `proc hashmap_new` signature `proc hashmap_new<K, V>() -> HashMap<K, V> {` example `collections.hashmap_new(...)` stability `stable`
- `proc hashmap_find_index` signature `proc hashmap_find_index<K, V>(map: ref HashMap<K, V>, key: K) -> Option<usize> {` example `collections.hashmap_find_index(...)` stability `stable`
- `proc hashmap_insert` signature `proc hashmap_insert<K, V>(map: ref mut HashMap<K, V>, key: K, value: V) -> Option<V> {` example `collections.hashmap_insert(...)` stability `stable`
- `proc hashmap_get` signature `proc hashmap_get<K, V>(map: ref HashMap<K, V>, key: K) -> Option<ref V> {` example `collections.hashmap_get(...)` stability `stable`
- `proc hashmap_remove` signature `proc hashmap_remove<K, V>(map: ref mut HashMap<K, V>, key: K) -> Option<V> {` example `collections.hashmap_remove(...)` stability `stable`
- `proc hashmap_len` signature `proc hashmap_len<K, V>(map: HashMap<K, V>) -> usize { give map.length; }` example `collections.hashmap_len(...)` stability `stable`
- `proc hashset_new` signature `proc hashset_new<T>() -> HashSet<T> { give HashSet<T> { map: hashmap_new<T, bool>() }; }` example `collections.hashset_new(...)` stability `stable`
- `proc hashset_insert` signature `proc hashset_insert<T>(set: ref mut HashSet<T>, value: T) -> bool { give is_none<bool>(hashmap_insert<T, bool>(&mut ((*set).map), value, true)); }` example `collections.hashset_insert(...)` stability `stable`
- `proc hashset_contains` signature `proc hashset_contains<T>(set: ref HashSet<T>, value: T) -> bool { give is_some<ref bool>(hashmap_get<T, bool>(&((*set).map), value)); }` example `collections.hashset_contains(...)` stability `stable`
- `proc hashset_remove` signature `proc hashset_remove<T>(set: ref mut HashSet<T>, value: T) -> bool { give is_some<bool>(hashmap_remove<T, bool>(&mut ((*set).map), value)); }` example `collections.hashset_remove(...)` stability `stable`
- `proc btreemap_new` signature `proc btreemap_new<K, V>() -> BTreeMap<K, V> { give BTreeMap<K, V> { entries: vec_new<(K, V)>(), length: 0 }; }` example `collections.btreemap_new(...)` stability `stable`
- `proc btreemap_find_index` signature `proc btreemap_find_index<K, V>(map: ref BTreeMap<K, V>, key: K) -> Option<usize> {` example `collections.btreemap_find_index(...)` stability `stable`
- `proc btreemap_insert` signature `proc btreemap_insert<K, V>(map: ref mut BTreeMap<K, V>, key: K, value: V) -> Option<V> {` example `collections.btreemap_insert(...)` stability `stable`
- `proc btreemap_get` signature `proc btreemap_get<K, V>(map: ref BTreeMap<K, V>, key: K) -> Option<ref V> {` example `collections.btreemap_get(...)` stability `stable`
- `proc btreemap_remove` signature `proc btreemap_remove<K, V>(map: ref mut BTreeMap<K, V>, key: K) -> Option<V> {` example `collections.btreemap_remove(...)` stability `stable`
- `proc btreemap_iter` signature `proc btreemap_iter<K, V>(map: BTreeMap<K, V>) -> Iterator<(K, V)> { give vec_iter<(K, V)>(map.entries); }` example `collections.btreemap_iter(...)` stability `stable`
- `proc btreeset_new` signature `proc btreeset_new<T>() -> BTreeSet<T> { give BTreeSet<T> { map: btreemap_new<T, bool>() }; }` example `collections.btreeset_new(...)` stability `stable`
- `proc btreeset_insert` signature `proc btreeset_insert<T>(set: ref mut BTreeSet<T>, value: T) -> bool { give is_none<bool>(btreemap_insert<T, bool>(&mut ((*set).map), value, true)); }` example `collections.btreeset_insert(...)` stability `stable`
- `proc btreeset_contains` signature `proc btreeset_contains<T>(set: ref BTreeSet<T>, value: T) -> bool { give is_some<ref bool>(btreemap_get<T, bool>(&((*set).map), value)); }` example `collections.btreeset_contains(...)` stability `stable`
- `proc btreeset_remove` signature `proc btreeset_remove<T>(set: ref mut BTreeSet<T>, value: T) -> bool { give is_some<bool>(btreemap_remove<T, bool>(&mut ((*set).map), value)); }` example `collections.btreeset_remove(...)` stability `stable`
- `proc deque_new` signature `proc deque_new<T>() -> Deque<T> { give Deque<T> { data: vec_new<T>(), head: 0, length: 0 }; }` example `collections.deque_new(...)` stability `stable`
- `proc deque_push_back` signature `proc deque_push_back<T>(deque: ref mut Deque<T>, value: T) { vec_push<T>(&mut ((*deque).data), value); set (*deque).length = (*deque).length + 1; }` example `collections.deque_push_back(...)` stability `stable`
- `proc deque_push_front` signature `proc deque_push_front<T>(deque: ref mut Deque<T>, value: T) { vec_insert<T>(&mut ((*deque).data), 0, value); set (*deque).length = (*deque).length + 1; }` example `collections.deque_push_front(...)` stability `stable`
- `proc deque_pop_back` signature `proc deque_pop_back<T>(deque: ref mut Deque<T>) -> Option<T> {` example `collections.deque_pop_back(...)` stability `stable`
- `proc deque_pop_front` signature `proc deque_pop_front<T>(deque: ref mut Deque<T>) -> Option<T> {` example `collections.deque_pop_front(...)` stability `stable`
- `proc deque_len` signature `proc deque_len<T>(deque: Deque<T>) -> usize { give deque.length; }` example `collections.deque_len(...)` stability `stable`
- `proc smallvec_new` signature `proc smallvec_new<T, N>() -> SmallVec<T, N> { give SmallVec<T, N> { inline_len: 0, spilled: vec_new<T>() }; }` example `collections.smallvec_new(...)` stability `stable`
- `proc smallvec_len` signature `proc smallvec_len<T, N>(value: SmallVec<T, N>) -> usize { give value.inline_len + vec_len<T>(value.spilled); }` example `collections.smallvec_len(...)` stability `stable`
- `proc smallvec_push` signature `proc smallvec_push<T, N>(value: ref mut SmallVec<T, N>, item: T) {` example `collections.smallvec_push(...)` stability `stable`
- `proc smallvec_pop` signature `proc smallvec_pop<T, N>(value: ref mut SmallVec<T, N>) -> Option<T> { give vec_pop<T>(&mut ((*value).spilled)); }` example `collections.smallvec_pop(...)` stability `stable`

## `src/vitte/stdlib/alloc/rc.vitl`

Stability: `stable`

- `form Rc` signature `form Rc<T> { ptr: ref T }` example `rc.Rc` stability `stable`
- `form Weak` signature `form Weak<T> { ptr: ref T }` example `rc.Weak` stability `stable`
- `proc rc_new` signature `proc rc_new<T>(value: T) -> Rc<T> { give compiler_rc_new<T>(value); }` example `rc.rc_new(...)` stability `stable`
- `proc rc_clone` signature `proc rc_clone<T>(value: Rc<T>) -> Rc<T> { give compiler_rc_clone<T>(value); }` example `rc.rc_clone(...)` stability `stable`
- `proc rc_strong_count` signature `proc rc_strong_count<T>(value: Rc<T>) -> usize { give compiler_rc_strong_count<T>(value); }` example `rc.rc_strong_count(...)` stability `stable`
- `proc rc_weak_count` signature `proc rc_weak_count<T>(value: Rc<T>) -> usize { give compiler_rc_weak_count<T>(value); }` example `rc.rc_weak_count(...)` stability `stable`
- `proc rc_downgrade` signature `proc rc_downgrade<T>(value: Rc<T>) -> Weak<T> { give compiler_rc_downgrade<T>(value); }` example `rc.rc_downgrade(...)` stability `stable`
- `proc weak_upgrade` signature `proc weak_upgrade<T>(value: Weak<T>) -> Option<Rc<T>> { give compiler_weak_upgrade<T>(value); }` example `rc.weak_upgrade(...)` stability `stable`

## `src/vitte/stdlib/alloc/string.vitl`

Stability: `stable`

- `form String` signature `form String {` example `string.String` stability `stable`
- `proc string_new` signature `proc string_new() -> String {` example `string.string_new(...)` stability `stable`
- `proc string_from_utf8` signature `proc string_from_utf8(bytes: Vec<byte>) -> Option<String> {` example `string.string_from_utf8(...)` stability `stable`
- `proc string_len` signature `proc string_len(value: String) -> usize { give vec_len<byte>(value.bytes); }` example `string.string_len(...)` stability `stable`
- `proc string_capacity` signature `proc string_capacity(value: String) -> usize { give vec_capacity<byte>(value.bytes); }` example `string.string_capacity(...)` stability `stable`
- `proc string_push` signature `proc string_push(value: ref mut String, ch: rune) {` example `string.string_push(...)` stability `stable`
- `proc string_push_str` signature `proc string_push_str(value: ref mut String, text: string) {` example `string.string_push_str(...)` stability `stable`
- `proc string_concat` signature `proc string_concat(left: String, right: string) -> String {` example `string.string_concat(...)` stability `stable`
- `proc string_slice` signature `proc string_slice(value: ref String, start: usize, end: usize) -> Option<Utf8View> {` example `string.string_slice(...)` stability `stable`
- `proc string_reserve` signature `proc string_reserve(value: ref mut String, additional: usize) { vec_reserve<byte>(&mut ((*value).bytes), additional); }` example `string.string_reserve(...)` stability `stable`
- `proc string_as_utf8_view` signature `proc string_as_utf8_view(value: ref String) -> Utf8View { give compiler_owned_string_as_utf8_view(value); }` example `string.string_as_utf8_view(...)` stability `stable`

## `src/vitte/stdlib/alloc/vec.vitl`

Stability: `stable`

- `form Vec` signature `form Vec<T> {` example `vec.Vec` stability `stable`
- `proc vec_new` signature `proc vec_new<T>() -> Vec<T> {` example `vec.vec_new(...)` stability `stable`
- `proc vec_with_capacity` signature `proc vec_with_capacity<T>(capacity: usize) -> Vec<T> {` example `vec.vec_with_capacity(...)` stability `stable`
- `proc vec_len` signature `proc vec_len<T>(vec: Vec<T>) -> usize { give vec.length; }` example `vec.vec_len(...)` stability `stable`
- `proc vec_capacity` signature `proc vec_capacity<T>(vec: Vec<T>) -> usize { give vec.capacity; }` example `vec.vec_capacity(...)` stability `stable`
- `proc vec_reserve` signature `proc vec_reserve<T>(vec: ref mut Vec<T>, additional: usize) {` example `vec.vec_reserve(...)` stability `stable`
- `proc vec_push` signature `proc vec_push<T>(vec: ref mut Vec<T>, value: T) {` example `vec.vec_push(...)` stability `stable`
- `proc vec_pop` signature `proc vec_pop<T>(vec: ref mut Vec<T>) -> Option<T> { give compiler_vec_pop<T>(vec); }` example `vec.vec_pop(...)` stability `stable`
- `proc vec_insert` signature `proc vec_insert<T>(vec: ref mut Vec<T>, index: usize, value: T) { compiler_vec_insert<T>(vec, index, value); }` example `vec.vec_insert(...)` stability `stable`
- `proc vec_remove` signature `proc vec_remove<T>(vec: ref mut Vec<T>, index: usize) -> Option<T> { give compiler_vec_remove<T>(vec, index); }` example `vec.vec_remove(...)` stability `stable`
- `proc vec_get` signature `proc vec_get<T>(vec: Vec<T>, index: usize) -> Option<ref T> {` example `vec.vec_get(...)` stability `stable`
- `proc vec_iter` signature `proc vec_iter<T>(vec: Vec<T>) -> Iterator<ref T> { give compiler_vec_iter<T>(vec); }` example `vec.vec_iter(...)` stability `stable`
- `proc vec_drop` signature `proc vec_drop<T>(vec: ref mut Vec<T>) {` example `vec.vec_drop(...)` stability `stable`

## `src/vitte/stdlib/async/async.vitl`

Stability: `stable`

- `form TimeoutResult` signature `form TimeoutResult<T> {` example `async.TimeoutResult` stability `stable`
- `form BatchProcessor` signature `form BatchProcessor<T, U> {` example `async.BatchProcessor` stability `stable`
- `form TaskPool` signature `form TaskPool {` example `async.TaskPool` stability `stable`
- `const GLOBAL_EXECUTOR` signature `const GLOBAL_EXECUTOR: Executor = Executor {` example `async.GLOBAL_EXECUTOR` stability `stable`
- `proc global_executor` signature `proc global_executor() -> Executor {` example `async.global_executor(...)` stability `stable`
- `proc async` signature `proc async<T>(body: proc) -> Future<T> {` example `async.async(...)` stability `stable`
- `proc await` signature `proc await<T>(fut: Future<T>) -> T {` example `async.await(...)` stability `stable`
- `proc try_await` signature `proc try_await<T>(fut: Future<T>, timeout_ms: int) -> bool {` example `async.try_await(...)` stability `stable`
- `proc spawn_task` signature `proc spawn_task<T>(fut: Future<T>) -> int {` example `async.spawn_task(...)` stability `stable`
- `proc join_all` signature `proc join_all(task_ids: [int]) -> bool {` example `async.join_all(...)` stability `stable`
- `proc async_sleep` signature `proc async_sleep(ms: int) -> Future<bool> {` example `async.async_sleep(...)` stability `stable`
- `proc init_executor` signature `proc init_executor(num_workers: int) -> bool {` example `async.init_executor(...)` stability `stable`
- `proc start_executor` signature `proc start_executor() -> int {` example `async.start_executor(...)` stability `stable`
- `proc stop_executor` signature `proc stop_executor() -> int {` example `async.stop_executor(...)` stability `stable`
- `proc executor_runtime_stats` signature `proc executor_runtime_stats() -> ExecutorStats {` example `async.executor_runtime_stats(...)` stability `stable`
- `proc create_channel` signature `proc create_channel<T>(capacity: int) -> Channel<T> {` example `async.create_channel(...)` stability `stable`
- `proc parallel_all` signature `proc parallel_all<T>(futures: [Future<T>]) -> [T] {` example `async.parallel_all(...)` stability `stable`
- `proc race_first` signature `proc race_first<T>(futures: [Future<T>]) -> T {` example `async.race_first(...)` stability `stable`
- `proc with_timeout` signature `proc with_timeout<T>(fut: Future<T>, timeout_ms: int) -> TimeoutResult<T> {` example `async.with_timeout(...)` stability `stable`
- `proc timeout_has_value` signature `proc timeout_has_value<T>(result: TimeoutResult<T>) -> bool {` example `async.timeout_has_value(...)` stability `stable`
- `proc timeout_value` signature `proc timeout_value<T>(result: TimeoutResult<T>) -> T {` example `async.timeout_value(...)` stability `stable`
- `proc retry_async` signature `proc retry_async<T>(operation_id: int, max_attempts: int, backoff_ms: int) -> Future<T> {` example `async.retry_async(...)` stability `stable`
- `proc batch_processor_new` signature `proc batch_processor_new<T, U>(size: int, processor_id: int) -> BatchProcessor<T, U> {` example `async.batch_processor_new(...)` stability `stable`
- `proc batch_add` signature `proc batch_add<T, U>(bp: BatchProcessor<T, U>, item: T) -> bool {` example `async.batch_add(...)` stability `stable`
- `proc task_pool_new` signature `proc task_pool_new(num_workers: int) -> TaskPool {` example `async.task_pool_new(...)` stability `stable`
- `proc task_pool_submit` signature `proc task_pool_submit(pool: TaskPool, task_id: int) -> bool {` example `async.task_pool_submit(...)` stability `stable`
- `proc task_pool_wait_all` signature `proc task_pool_wait_all(pool: TaskPool) -> bool {` example `async.task_pool_wait_all(...)` stability `stable`
- `proc task_pool_shutdown` signature `proc task_pool_shutdown(pool: TaskPool) -> bool {` example `async.task_pool_shutdown(...)` stability `stable`
- `proc sleep_ms` signature `proc sleep_ms(ms: int) -> bool {` example `async.sleep_ms(...)` stability `stable`
- `proc async_version` signature `proc async_version() -> string {` example `async.async_version(...)` stability `stable`
- `proc async_ready` signature `proc async_ready() -> bool {` example `async.async_ready(...)` stability `stable`
- `proc async_selftest_value` signature `proc async_selftest_value() -> int {` example `async.async_selftest_value(...)` stability `stable`
- `proc async_selftest` signature `proc async_selftest() -> bool {` example `async.async_selftest(...)` stability `stable`

## `src/vitte/stdlib/async/channel.vitl`

Stability: `stable`

- `form Queue` signature `form Queue<T> {` example `channel.Queue` stability `stable`
- `form Channel` signature `form Channel<T> {` example `channel.Channel` stability `stable`
- `form Select` signature `form Select<T> {` example `channel.Select` stability `stable`
- `proc channel_new` signature `proc channel_new<T>(capacity: int) -> Channel<T> {` example `channel.channel_new(...)` stability `stable`
- `proc channel_send` signature `proc channel_send<T>(ch: Channel<T>, value: T) -> bool {` example `channel.channel_send(...)` stability `stable`
- `proc channel_recv` signature `proc channel_recv<T>(ch: Channel<T>) -> T {` example `channel.channel_recv(...)` stability `stable`
- `proc channel_try_recv` signature `proc channel_try_recv<T>(ch: Channel<T>) -> T {` example `channel.channel_try_recv(...)` stability `stable`
- `proc channel_is_closed` signature `proc channel_is_closed<T>(ch: Channel<T>) -> bool {` example `channel.channel_is_closed(...)` stability `stable`
- `proc channel_close_sender` signature `proc channel_close_sender<T>(ch: Channel<T>) -> bool {` example `channel.channel_close_sender(...)` stability `stable`
- `proc channel_close_receiver` signature `proc channel_close_receiver<T>(ch: Channel<T>) -> bool {` example `channel.channel_close_receiver(...)` stability `stable`
- `proc channel_len` signature `proc channel_len<T>(ch: Channel<T>) -> int {` example `channel.channel_len(...)` stability `stable`
- `proc channel_select` signature `proc channel_select<T>(channels: [Channel<T>]) -> Select<T> {` example `channel.channel_select(...)` stability `stable`
- `proc queue_new` signature `proc queue_new<T>() -> Queue<T> {` example `channel.queue_new(...)` stability `stable`
- `proc queue_push` signature `proc queue_push<T>(q: Queue<T>, value: T) -> bool {` example `channel.queue_push(...)` stability `stable`
- `proc queue_pop` signature `proc queue_pop<T>(q: Queue<T>) -> T {` example `channel.queue_pop(...)` stability `stable`
- `proc queue_len` signature `proc queue_len<T>(q: Queue<T>) -> int {` example `channel.queue_len(...)` stability `stable`
- `proc queue_rest` signature `proc queue_rest<T>(items: [T]) -> [T] {` example `channel.queue_rest(...)` stability `stable`
- `proc mutex_new` signature `proc mutex_new() -> int {` example `channel.mutex_new(...)` stability `stable`
- `proc mutex_lock` signature `proc mutex_lock(id: int) -> bool {` example `channel.mutex_lock(...)` stability `stable`
- `proc mutex_unlock` signature `proc mutex_unlock(id: int) -> bool {` example `channel.mutex_unlock(...)` stability `stable`
- `proc cond_new` signature `proc cond_new() -> int {` example `channel.cond_new(...)` stability `stable`
- `proc cond_wait` signature `proc cond_wait(cond: int, mutex: int) -> bool {` example `channel.cond_wait(...)` stability `stable`
- `proc cond_signal` signature `proc cond_signal(cond: int) -> bool {` example `channel.cond_signal(...)` stability `stable`
- `proc cond_broadcast` signature `proc cond_broadcast(cond: int) -> bool {` example `channel.cond_broadcast(...)` stability `stable`
- `proc channel_version` signature `proc channel_version() -> string {` example `channel.channel_version(...)` stability `stable`
- `proc channel_ready` signature `proc channel_ready() -> bool {` example `channel.channel_ready(...)` stability `stable`
- `proc channel_selftest` signature `proc channel_selftest() -> bool {` example `channel.channel_selftest(...)` stability `stable`

## `src/vitte/stdlib/async/future.vitl`

Stability: `stable`

- `form Future` signature `form Future<T> {` example `future.Future` stability `stable`
- `proc future_new` signature `proc future_new<T>() -> Future<T> {` example `future.future_new(...)` stability `stable`
- `proc future_is_ready` signature `proc future_is_ready<T>(fut: Future<T>) -> bool {` example `future.future_is_ready(...)` stability `stable`
- `proc future_is_completed` signature `proc future_is_completed<T>(fut: Future<T>) -> bool {` example `future.future_is_completed(...)` stability `stable`
- `proc future_is_errored` signature `proc future_is_errored<T>(fut: Future<T>) -> bool {` example `future.future_is_errored(...)` stability `stable`
- `proc future_await` signature `proc future_await<T>(fut: Future<T>) -> T {` example `future.future_await(...)` stability `stable`
- `proc future_try_get` signature `proc future_try_get<T>(fut: Future<T>) -> T {` example `future.future_try_get(...)` stability `stable`
- `proc future_resolve` signature `proc future_resolve<T>(fut: Future<T>, value: T) -> bool {` example `future.future_resolve(...)` stability `stable`
- `proc future_reject` signature `proc future_reject<T>(fut: Future<T>, error: string) -> bool {` example `future.future_reject(...)` stability `stable`
- `proc future_error_message` signature `proc future_error_message<T>(fut: Future<T>) -> string {` example `future.future_error_message(...)` stability `stable`
- `proc future_map` signature `proc future_map<T, U>(fut: Future<T>, mapper: proc) -> Future<U> {` example `future.future_map(...)` stability `stable`
- `proc future_chain` signature `proc future_chain<T, U>(fut: Future<T>, then: proc) -> Future<U> {` example `future.future_chain(...)` stability `stable`
- `proc future_race` signature `proc future_race<T>(futures: [Future<T>]) -> T {` example `future.future_race(...)` stability `stable`
- `proc future_all` signature `proc future_all<T>(futures: [Future<T>]) -> [T] {` example `future.future_all(...)` stability `stable`
- `proc future_from_value` signature `proc future_from_value<T>(value: T) -> Future<T> {` example `future.future_from_value(...)` stability `stable`
- `proc future_from_error` signature `proc future_from_error<T>(error: string) -> Future<T> {` example `future.future_from_error(...)` stability `stable`
- `proc yield_cpu` signature `proc yield_cpu() -> bool {` example `future.yield_cpu(...)` stability `stable`
- `proc mutex_new` signature `proc mutex_new() -> int {` example `future.mutex_new(...)` stability `stable`
- `proc mutex_lock` signature `proc mutex_lock(id: int) -> bool {` example `future.mutex_lock(...)` stability `stable`
- `proc mutex_unlock` signature `proc mutex_unlock(id: int) -> bool {` example `future.mutex_unlock(...)` stability `stable`
- `proc future_version` signature `proc future_version() -> string {` example `future.future_version(...)` stability `stable`
- `proc future_ready` signature `proc future_ready() -> bool {` example `future.future_ready(...)` stability `stable`
- `proc future_selftest_add_one` signature `proc future_selftest_add_one(value: int) -> int {` example `future.future_selftest_add_one(...)` stability `stable`
- `proc future_selftest_double` signature `proc future_selftest_double(value: int) -> Future<int> {` example `future.future_selftest_double(...)` stability `stable`
- `proc future_selftest` signature `proc future_selftest() -> bool {` example `future.future_selftest(...)` stability `stable`

## `src/vitte/stdlib/collections/queue.vitl`

Stability: `stable`

- `form Queue` signature `form Queue {` example `queue.Queue` stability `stable`
- `proc _repeat_i64` signature `proc _repeat_i64(value: i64, count: i32) -> [i64] {` example `queue._repeat_i64(...)` stability `stable`
- `proc queue_new` signature `proc queue_new(capacity: i32) -> Queue {` example `queue.queue_new(...)` stability `stable`
- `proc queue_reserve` signature `proc queue_reserve(q: Queue, new_cap: i32) {` example `queue.queue_reserve(...)` stability `stable`
- `proc _queue_resize` signature `proc _queue_resize(q: Queue) {` example `queue._queue_resize(...)` stability `stable`
- `proc _queue_shrink` signature `proc _queue_shrink(q: Queue) {` example `queue._queue_shrink(...)` stability `stable`
- `proc queue_enqueue` signature `proc queue_enqueue(q: Queue, value: i64) -> int {` example `queue.queue_enqueue(...)` stability `stable`
- `proc queue_dequeue` signature `proc queue_dequeue(q: Queue) -> i64 {` example `queue.queue_dequeue(...)` stability `stable`
- `proc queue_push_front` signature `proc queue_push_front(q: Queue, value: i64) -> int {` example `queue.queue_push_front(...)` stability `stable`
- `proc queue_pop_back` signature `proc queue_pop_back(q: Queue) -> i64 {` example `queue.queue_pop_back(...)` stability `stable`
- `proc queue_front` signature `proc queue_front(q: Queue) -> i64 {` example `queue.queue_front(...)` stability `stable`
- `proc queue_back` signature `proc queue_back(q: Queue) -> i64 {` example `queue.queue_back(...)` stability `stable`
- `proc queue_empty` signature `proc queue_empty(q: Queue) -> int {` example `queue.queue_empty(...)` stability `stable`
- `proc queue_size` signature `proc queue_size(q: Queue) -> i32 {` example `queue.queue_size(...)` stability `stable`
- `proc queue_clear` signature `proc queue_clear(q: Queue) {` example `queue.queue_clear(...)` stability `stable`
- `proc queue_is_full` signature `proc queue_is_full(q: Queue) -> int {` example `queue.queue_is_full(...)` stability `stable`
- `proc queue_get` signature `proc queue_get(q: Queue, idx: i64) -> i64 {` example `queue.queue_get(...)` stability `stable`
- `proc queue_to_array` signature `proc queue_to_array(q: Queue) -> [i64] {` example `queue.queue_to_array(...)` stability `stable`
- `proc __len__` signature `proc __len__(q: Queue) -> int {` example `queue.__len__(...)` stability `stable`
- `proc __getitem__` signature `proc __getitem__(q: Queue, idx: i64) -> i64 {` example `queue.__getitem__(...)` stability `stable`
- `proc __iter__` signature `proc __iter__(q: Queue) -> [i64] {` example `queue.__iter__(...)` stability `stable`

## `src/vitte/stdlib/collections/vector.vitl`

Stability: `stable`

- `form Vector` signature `form Vector {` example `vector.Vector` stability `stable`
- `proc _repeat_i64` signature `proc _repeat_i64(value: i64, count: i32) -> [i64] {` example `vector._repeat_i64(...)` stability `stable`
- `proc vector_new` signature `proc vector_new(capacity: i32) -> Vector {` example `vector.vector_new(...)` stability `stable`
- `proc _vector_resize` signature `proc _vector_resize(v: Vector) {` example `vector._vector_resize(...)` stability `stable`
- `proc _vector_shrink` signature `proc _vector_shrink(v: Vector) {` example `vector._vector_shrink(...)` stability `stable`
- `proc vector_push` signature `proc vector_push(v: Vector, value: i64) -> int {` example `vector.vector_push(...)` stability `stable`
- `proc vector_push_unchecked` signature `proc vector_push_unchecked(v: Vector, value: i64) {` example `vector.vector_push_unchecked(...)` stability `stable`
- `proc vector_insert` signature `proc vector_insert(v: Vector, idx: i32, value: i64) -> int {` example `vector.vector_insert(...)` stability `stable`
- `proc vector_pop` signature `proc vector_pop(v: Vector) -> i64 {` example `vector.vector_pop(...)` stability `stable`
- `proc vector_remove_at` signature `proc vector_remove_at(v: Vector, idx: i32) -> i64 {` example `vector.vector_remove_at(...)` stability `stable`
- `proc vector_at` signature `proc vector_at(v: Vector, index: i32) -> i64 {` example `vector.vector_at(...)` stability `stable`
- `proc vector_get_unchecked` signature `proc vector_get_unchecked(v: Vector, index: i32) -> i64 {` example `vector.vector_get_unchecked(...)` stability `stable`
- `proc vector_set` signature `proc vector_set(v: Vector, index: i32, value: i64) -> int {` example `vector.vector_set(...)` stability `stable`
- `proc vector_swap` signature `proc vector_swap(v: Vector, i: i32, j: i32) {` example `vector.vector_swap(...)` stability `stable`
- `proc vector_reverse` signature `proc vector_reverse(v: Vector) {` example `vector.vector_reverse(...)` stability `stable`
- `proc vector_reserve` signature `proc vector_reserve(v: Vector, new_cap: i32) {` example `vector.vector_reserve(...)` stability `stable`
- `proc vector_clear` signature `proc vector_clear(v: Vector) {` example `vector.vector_clear(...)` stability `stable`
- `proc vector_push_many` signature `proc vector_push_many(v: Vector, values: [i64]) {` example `vector.vector_push_many(...)` stability `stable`
- `proc vector_extend` signature `proc vector_extend(v: Vector, other: Vector) {` example `vector.vector_extend(...)` stability `stable`
- `proc vector_slice` signature `proc vector_slice(v: Vector, start: i32, end: i32) -> [i64] {` example `vector.vector_slice(...)` stability `stable`
- `proc vector_find` signature `proc vector_find(v: Vector, value: i64) -> i32 {` example `vector.vector_find(...)` stability `stable`
- `proc vector_equals` signature `proc vector_equals(a: Vector, b: Vector) -> int {` example `vector.vector_equals(...)` stability `stable`
- `proc vector_truncate` signature `proc vector_truncate(v: Vector, n: i32) {` example `vector.vector_truncate(...)` stability `stable`
- `proc vector_clone` signature `proc vector_clone(v: Vector) -> Vector {` example `vector.vector_clone(...)` stability `stable`
- `proc vector_size` signature `proc vector_size(v: Vector) -> i32 {` example `vector.vector_size(...)` stability `stable`
- `proc vector_capacity` signature `proc vector_capacity(v: Vector) -> i32 {` example `vector.vector_capacity(...)` stability `stable`
- `proc vector_empty` signature `proc vector_empty(v: Vector) -> int {` example `vector.vector_empty(...)` stability `stable`
- `proc vector_to_array` signature `proc vector_to_array(v: Vector) -> [i64] {` example `vector.vector_to_array(...)` stability `stable`
- `proc __len__` signature `proc __len__(v: Vector) -> int {` example `vector.__len__(...)` stability `stable`
- `proc __getitem__` signature `proc __getitem__(v: Vector, index: i64) -> i64 {` example `vector.__getitem__(...)` stability `stable`
- `proc __iter__` signature `proc __iter__(v: Vector) -> [i64] {` example `vector.__iter__(...)` stability `stable`

## `src/vitte/stdlib/compression/algorithms.vitl`

Stability: `stable`

- `const RLE_MARK` signature `const RLE_MARK: i32 = -2147483648` example `algorithms.RLE_MARK` stability `stable`
- `const LZ_WINDOW` signature `const LZ_WINDOW: i32 = 32` example `algorithms.LZ_WINDOW` stability `stable`
- `const LZ_LOOKAHEAD` signature `const LZ_LOOKAHEAD: i32 = 16` example `algorithms.LZ_LOOKAHEAD` stability `stable`
- `form LZToken` signature `form LZToken {` example `algorithms.LZToken` stability `stable`
- `form CompressionAlgorithmsManifest` signature `form CompressionAlgorithmsManifest {` example `algorithms.CompressionAlgorithmsManifest` stability `stable`
- `form CompressionAlgorithmsHealth` signature `form CompressionAlgorithmsHealth {` example `algorithms.CompressionAlgorithmsHealth` stability `stable`
- `form CompressionAlgorithmsSummary` signature `form CompressionAlgorithmsSummary {` example `algorithms.CompressionAlgorithmsSummary` stability `stable`
- `proc compress_rle` signature `proc compress_rle(data: [i32]) -> [i32] {` example `algorithms.compress_rle(...)` stability `stable`
- `proc decompress_rle` signature `proc decompress_rle(data: [i32]) -> [i32] {` example `algorithms.decompress_rle(...)` stability `stable`
- `proc compress_lz77` signature `proc compress_lz77(data: [i32]) -> [LZToken] {` example `algorithms.compress_lz77(...)` stability `stable`
- `proc decompress_lz77` signature `proc decompress_lz77(tokens: [LZToken]) -> [i32] {` example `algorithms.decompress_lz77(...)` stability `stable`
- `proc compress_huffman` signature `proc compress_huffman(data: [i32]) -> [i32] {` example `algorithms.compress_huffman(...)` stability `stable`
- `proc decompress_huffman` signature `proc decompress_huffman(data: [i32]) -> [i32] {` example `algorithms.decompress_huffman(...)` stability `stable`
- `proc compression_algorithms_version` signature `proc compression_algorithms_version() -> string {` example `algorithms.compression_algorithms_version(...)` stability `stable`
- `proc compression_algorithms_ready` signature `proc compression_algorithms_ready() -> bool {` example `algorithms.compression_algorithms_ready(...)` stability `stable`
- `proc compression_algorithms_manifest` signature `proc compression_algorithms_manifest() -> CompressionAlgorithmsManifest {` example `algorithms.compression_algorithms_manifest(...)` stability `stable`
- `proc compression_algorithms_health` signature `proc compression_algorithms_health() -> CompressionAlgorithmsHealth {` example `algorithms.compression_algorithms_health(...)` stability `stable`
- `proc compression_algorithms_summary` signature `proc compression_algorithms_summary() -> CompressionAlgorithmsSummary {` example `algorithms.compression_algorithms_summary(...)` stability `stable`
- `proc compression_algorithms_selftest` signature `proc compression_algorithms_selftest() -> bool {` example `algorithms.compression_algorithms_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/brotli.vitl`

Stability: `stable`

- `form BrotliManifest` signature `form BrotliManifest {` example `brotli.BrotliManifest` stability `stable`
- `form BrotliHealth` signature `form BrotliHealth {` example `brotli.BrotliHealth` stability `stable`
- `form BrotliSummary` signature `form BrotliSummary {` example `brotli.BrotliSummary` stability `stable`
- `proc _slice_text` signature `proc _slice_text(text: string, start: i32, end: i32) -> string {` example `brotli._slice_text(...)` stability `stable`
- `proc _strip_prefix` signature `proc _strip_prefix(text: string, prefix: string) -> string {` example `brotli._strip_prefix(...)` stability `stable`
- `proc _strip_quality_prefix` signature `proc _strip_quality_prefix(text: string) -> string {` example `brotli._strip_quality_prefix(...)` stability `stable`
- `proc normalize_quality` signature `proc normalize_quality(quality: int) -> int {` example `brotli.normalize_quality(...)` stability `stable`
- `proc compress` signature `proc compress(data: string) -> string {` example `brotli.compress(...)` stability `stable`
- `proc decompress` signature `proc decompress(data: string) -> string {` example `brotli.decompress(...)` stability `stable`
- `proc compress_with_quality` signature `proc compress_with_quality(data: string, quality: int) -> string {` example `brotli.compress_with_quality(...)` stability `stable`
- `proc compress_bytes` signature `proc compress_bytes(data: [int]) -> [int] {` example `brotli.compress_bytes(...)` stability `stable`
- `proc decompress_bytes` signature `proc decompress_bytes(data: [int]) -> [int] {` example `brotli.decompress_bytes(...)` stability `stable`
- `proc brotli_version` signature `proc brotli_version() -> string {` example `brotli.brotli_version(...)` stability `stable`
- `proc brotli_ready` signature `proc brotli_ready() -> bool {` example `brotli.brotli_ready(...)` stability `stable`
- `proc brotli_manifest` signature `proc brotli_manifest() -> BrotliManifest {` example `brotli.brotli_manifest(...)` stability `stable`
- `proc brotli_health` signature `proc brotli_health() -> BrotliHealth {` example `brotli.brotli_health(...)` stability `stable`
- `proc brotli_summary` signature `proc brotli_summary() -> BrotliSummary {` example `brotli.brotli_summary(...)` stability `stable`
- `proc get_compressed_size` signature `proc get_compressed_size(data: string) -> int {` example `brotli.get_compressed_size(...)` stability `stable`
- `proc compression_ratio` signature `proc compression_ratio(original: string, compressed: string) -> f64 {` example `brotli.compression_ratio(...)` stability `stable`
- `proc brotli_selftest` signature `proc brotli_selftest() -> bool {` example `brotli.brotli_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/deflate.vitl`

Stability: `stable`

- `const Z_NO_COMPRESSION` signature `const Z_NO_COMPRESSION: i32 = 0` example `deflate.Z_NO_COMPRESSION` stability `stable`
- `const Z_FAST_COMPRESSION` signature `const Z_FAST_COMPRESSION: i32 = 1` example `deflate.Z_FAST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_COMPRESSION` signature `const Z_DEFAULT_COMPRESSION: i32 = 6` example `deflate.Z_DEFAULT_COMPRESSION` stability `stable`
- `const Z_BEST_COMPRESSION` signature `const Z_BEST_COMPRESSION: i32 = 9` example `deflate.Z_BEST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_STRATEGY` signature `const Z_DEFAULT_STRATEGY: i32 = 0` example `deflate.Z_DEFAULT_STRATEGY` stability `stable`
- `const Z_RLE` signature `const Z_RLE: i32 = 3` example `deflate.Z_RLE` stability `stable`
- `form DeflateContext` signature `form DeflateContext {` example `deflate.DeflateContext` stability `stable`
- `form DeflateManifest` signature `form DeflateManifest {` example `deflate.DeflateManifest` stability `stable`
- `form DeflateHealth` signature `form DeflateHealth {` example `deflate.DeflateHealth` stability `stable`
- `form DeflateSummary` signature `form DeflateSummary {` example `deflate.DeflateSummary` stability `stable`
- `proc _slice_text` signature `proc _slice_text(text: string, start: i32, end: i32) -> string {` example `deflate._slice_text(...)` stability `stable`
- `proc deflate_version` signature `proc deflate_version() -> string {` example `deflate.deflate_version(...)` stability `stable`
- `proc deflate_ready` signature `proc deflate_ready() -> bool {` example `deflate.deflate_ready(...)` stability `stable`
- `proc deflate_manifest` signature `proc deflate_manifest() -> DeflateManifest {` example `deflate.deflate_manifest(...)` stability `stable`
- `proc deflate_health` signature `proc deflate_health() -> DeflateHealth {` example `deflate.deflate_health(...)` stability `stable`
- `proc deflate_summary` signature `proc deflate_summary() -> DeflateSummary {` example `deflate.deflate_summary(...)` stability `stable`
- `proc compress_deflate` signature `proc compress_deflate(data: [i32], level: i32) -> [i32] {` example `deflate.compress_deflate(...)` stability `stable`
- `proc decompress_deflate` signature `proc decompress_deflate(data: [i32]) -> [i32] {` example `deflate.decompress_deflate(...)` stability `stable`
- `proc compress_zlib` signature `proc compress_zlib(data: [i32], level: i32) -> [i32] {` example `deflate.compress_zlib(...)` stability `stable`
- `proc decompress_zlib` signature `proc decompress_zlib(data: [i32]) -> [i32] {` example `deflate.decompress_zlib(...)` stability `stable`
- `proc compress_gzip` signature `proc compress_gzip(data: [i32], level: i32) -> [i32] {` example `deflate.compress_gzip(...)` stability `stable`
- `proc decompress_gzip` signature `proc decompress_gzip(data: [i32]) -> [i32] {` example `deflate.decompress_gzip(...)` stability `stable`
- `proc deflate_selftest` signature `proc deflate_selftest() -> bool {` example `deflate.deflate_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/huffman.vitl`

Stability: `stable`

- `form FrequencyEntry` signature `form FrequencyEntry {` example `huffman.FrequencyEntry` stability `stable`
- `form HuffmanManifest` signature `form HuffmanManifest {` example `huffman.HuffmanManifest` stability `stable`
- `form HuffmanHealth` signature `form HuffmanHealth {` example `huffman.HuffmanHealth` stability `stable`
- `form HuffmanSummary` signature `form HuffmanSummary {` example `huffman.HuffmanSummary` stability `stable`
- `proc _slice_text` signature `proc _slice_text(text: string, start: i32, end: i32) -> string {` example `huffman._slice_text(...)` stability `stable`
- `proc _strip_prefix` signature `proc _strip_prefix(text: string, prefix: string) -> string {` example `huffman._strip_prefix(...)` stability `stable`
- `proc encode` signature `proc encode(data: string) -> string {` example `huffman.encode(...)` stability `stable`
- `proc decode` signature `proc decode(data: string) -> string {` example `huffman.decode(...)` stability `stable`
- `proc build_frequency_table` signature `proc build_frequency_table(data: string) -> [FrequencyEntry] {` example `huffman.build_frequency_table(...)` stability `stable`
- `proc build_huffman_tree` signature `proc build_huffman_tree(freq: [FrequencyEntry]) -> string {` example `huffman.build_huffman_tree(...)` stability `stable`
- `proc generate_codes` signature `proc generate_codes(tree: string) -> [string] {` example `huffman.generate_codes(...)` stability `stable`
- `proc adaptive_huffman_encode` signature `proc adaptive_huffman_encode(data: string) -> string {` example `huffman.adaptive_huffman_encode(...)` stability `stable`
- `proc adaptive_huffman_decode` signature `proc adaptive_huffman_decode(data: string) -> string {` example `huffman.adaptive_huffman_decode(...)` stability `stable`
- `proc huffman_version` signature `proc huffman_version() -> string {` example `huffman.huffman_version(...)` stability `stable`
- `proc huffman_ready` signature `proc huffman_ready() -> bool {` example `huffman.huffman_ready(...)` stability `stable`
- `proc huffman_manifest` signature `proc huffman_manifest() -> HuffmanManifest {` example `huffman.huffman_manifest(...)` stability `stable`
- `proc huffman_health` signature `proc huffman_health() -> HuffmanHealth {` example `huffman.huffman_health(...)` stability `stable`
- `proc huffman_summary` signature `proc huffman_summary() -> HuffmanSummary {` example `huffman.huffman_summary(...)` stability `stable`
- `proc huffman_selftest` signature `proc huffman_selftest() -> bool {` example `huffman.huffman_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/interface.vitl`

Stability: `stable`

- `const ALGO_DEFLATE` signature `const ALGO_DEFLATE: i32 = 1` example `interface.ALGO_DEFLATE` stability `stable`
- `const ALGO_GZIP` signature `const ALGO_GZIP: i32 = 2` example `interface.ALGO_GZIP` stability `stable`
- `const ALGO_ZLIB` signature `const ALGO_ZLIB: i32 = 3` example `interface.ALGO_ZLIB` stability `stable`
- `const ALGO_RLE` signature `const ALGO_RLE: i32 = 4` example `interface.ALGO_RLE` stability `stable`
- `const ALGO_HUFFMAN` signature `const ALGO_HUFFMAN: i32 = 5` example `interface.ALGO_HUFFMAN` stability `stable`
- `const ALGO_LZ77` signature `const ALGO_LZ77: i32 = 6` example `interface.ALGO_LZ77` stability `stable`
- `const ALGO_BROTLI` signature `const ALGO_BROTLI: i32 = 7` example `interface.ALGO_BROTLI` stability `stable`
- `const ERR_INVALID_ALGO` signature `const ERR_INVALID_ALGO: i32 = -1` example `interface.ERR_INVALID_ALGO` stability `stable`
- `form CompressionInterfaceManifest` signature `form CompressionInterfaceManifest {` example `interface.CompressionInterfaceManifest` stability `stable`
- `form CompressionInterfaceHealth` signature `form CompressionInterfaceHealth {` example `interface.CompressionInterfaceHealth` stability `stable`
- `form CompressionInterfaceSummary` signature `form CompressionInterfaceSummary {` example `interface.CompressionInterfaceSummary` stability `stable`
- `form Compressor` signature `form Compressor {` example `interface.Compressor` stability `stable`
- `form Decompressor` signature `form Decompressor {` example `interface.Decompressor` stability `stable`
- `proc compression_interface_version` signature `proc compression_interface_version() -> string {` example `interface.compression_interface_version(...)` stability `stable`
- `proc compression_interface_ready` signature `proc compression_interface_ready() -> bool {` example `interface.compression_interface_ready(...)` stability `stable`
- `proc compression_interface_manifest` signature `proc compression_interface_manifest() -> CompressionInterfaceManifest {` example `interface.compression_interface_manifest(...)` stability `stable`
- `proc compression_interface_health` signature `proc compression_interface_health() -> CompressionInterfaceHealth {` example `interface.compression_interface_health(...)` stability `stable`
- `proc compression_interface_summary` signature `proc compression_interface_summary() -> CompressionInterfaceSummary {` example `interface.compression_interface_summary(...)` stability `stable`
- `proc compressor_new` signature `proc compressor_new(level: i32) -> Compressor {` example `interface.compressor_new(...)` stability `stable`
- `proc compressor_set_algorithm` signature `proc compressor_set_algorithm(c: Compressor, algo: i32) -> int {` example `interface.compressor_set_algorithm(...)` stability `stable`
- `proc compressor_set_strategy` signature `proc compressor_set_strategy(c: Compressor, strategy: i32) -> int {` example `interface.compressor_set_strategy(...)` stability `stable`
- `proc compressor_compress` signature `proc compressor_compress(c: Compressor, data: [i32]) -> [i32] {` example `interface.compressor_compress(...)` stability `stable`
- `proc compressor_flush` signature `proc compressor_flush(c: Compressor) -> [i32] {` example `interface.compressor_flush(...)` stability `stable`
- `proc compressor_reset` signature `proc compressor_reset(c: Compressor) -> int {` example `interface.compressor_reset(...)` stability `stable`
- `proc decompressor_new` signature `proc decompressor_new() -> Decompressor {` example `interface.decompressor_new(...)` stability `stable`
- `proc decompressor_set_algorithm` signature `proc decompressor_set_algorithm(d: Decompressor, algo: i32) -> int {` example `interface.decompressor_set_algorithm(...)` stability `stable`
- `proc decompressor_decompress` signature `proc decompressor_decompress(d: Decompressor, data: [i32]) -> [i32] {` example `interface.decompressor_decompress(...)` stability `stable`
- `proc decompressor_flush` signature `proc decompressor_flush(d: Decompressor) -> [i32] {` example `interface.decompressor_flush(...)` stability `stable`
- `proc decompressor_reset` signature `proc decompressor_reset(d: Decompressor) -> int {` example `interface.decompressor_reset(...)` stability `stable`
- `proc compress` signature `proc compress(data: [i32], algo: i32, level: i32) -> [i32] {` example `interface.compress(...)` stability `stable`
- `proc decompress` signature `proc decompress(data: [i32], algo: i32) -> [i32] {` example `interface.decompress(...)` stability `stable`
- `proc compress_ex` signature `proc compress_ex(data: [i32], algo: i32, level: i32, strategy: i32) -> [i32] {` example `interface.compress_ex(...)` stability `stable`
- `proc compression_interface_selftest` signature `proc compression_interface_selftest() -> bool {` example `interface.compression_interface_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/lz.vitl`

Stability: `stable`

- `form LZManifest` signature `form LZManifest {` example `lz.LZManifest` stability `stable`
- `form LZHealth` signature `form LZHealth {` example `lz.LZHealth` stability `stable`
- `form LZSummary` signature `form LZSummary {` example `lz.LZSummary` stability `stable`
- `proc _strip_prefix` signature `proc _strip_prefix(text: string, prefix: string) -> string {` example `lz._strip_prefix(...)` stability `stable`
- `proc _slice_text` signature `proc _slice_text(text: string, start: i32, end: i32) -> string {` example `lz._slice_text(...)` stability `stable`
- `proc compress` signature `proc compress(data: string) -> string {` example `lz.compress(...)` stability `stable`
- `proc decompress` signature `proc decompress(data: string) -> string {` example `lz.decompress(...)` stability `stable`
- `proc lz77_compress` signature `proc lz77_compress(data: string) -> string {` example `lz.lz77_compress(...)` stability `stable`
- `proc lz77_decompress` signature `proc lz77_decompress(data: string) -> string {` example `lz.lz77_decompress(...)` stability `stable`
- `proc lz78_compress` signature `proc lz78_compress(data: string) -> string {` example `lz.lz78_compress(...)` stability `stable`
- `proc lz78_decompress` signature `proc lz78_decompress(data: string) -> string {` example `lz.lz78_decompress(...)` stability `stable`
- `proc find_longest_match` signature `proc find_longest_match(data: string, pos: int, window_size: int) -> string {` example `lz.find_longest_match(...)` stability `stable`
- `proc lz_version` signature `proc lz_version() -> string {` example `lz.lz_version(...)` stability `stable`
- `proc lz_ready` signature `proc lz_ready() -> bool {` example `lz.lz_ready(...)` stability `stable`
- `proc lz_manifest` signature `proc lz_manifest() -> LZManifest {` example `lz.lz_manifest(...)` stability `stable`
- `proc lz_health` signature `proc lz_health() -> LZHealth {` example `lz.lz_health(...)` stability `stable`
- `proc lz_summary` signature `proc lz_summary() -> LZSummary {` example `lz.lz_summary(...)` stability `stable`
- `proc lz_selftest` signature `proc lz_selftest() -> bool {` example `lz.lz_selftest(...)` stability `stable`

## `src/vitte/stdlib/compression/stats.vitl`

Stability: `stable`

- `const Z_FAST_COMPRESSION` signature `const Z_FAST_COMPRESSION: i32 = 1` example `stats.Z_FAST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_COMPRESSION` signature `const Z_DEFAULT_COMPRESSION: i32 = 6` example `stats.Z_DEFAULT_COMPRESSION` stability `stable`
- `const Z_BEST_COMPRESSION` signature `const Z_BEST_COMPRESSION: i32 = 9` example `stats.Z_BEST_COMPRESSION` stability `stable`
- `const Z_DEFAULT_STRATEGY` signature `const Z_DEFAULT_STRATEGY: i32 = 0` example `stats.Z_DEFAULT_STRATEGY` stability `stable`
- `const Z_RLE` signature `const Z_RLE: i32 = 3` example `stats.Z_RLE` stability `stable`
- `const ALGO_DEFLATE` signature `const ALGO_DEFLATE: i32 = 1` example `stats.ALGO_DEFLATE` stability `stable`
- `const ALGO_ZLIB` signature `const ALGO_ZLIB: i32 = 3` example `stats.ALGO_ZLIB` stability `stable`
- `proc get_compression_ratio` signature `proc get_compression_ratio(original_size: i64, compressed_size: i64) -> f64 {` example `stats.get_compression_ratio(...)` stability `stable`
- `proc detect_repetition_ratio` signature `proc detect_repetition_ratio(data: [i32]) -> f64 {` example `stats.detect_repetition_ratio(...)` stability `stable`
- `proc count_unique` signature `proc count_unique(data: [i32]) -> i32 {` example `stats.count_unique(...)` stability `stable`
- `proc estimate_entropy` signature `proc estimate_entropy(data: [i32]) -> f64 {` example `stats.estimate_entropy(...)` stability `stable`
- `proc estimate_compressed_size` signature `proc estimate_compressed_size(data: [i32]) -> i64 {` example `stats.estimate_compressed_size(...)` stability `stable`
- `proc get_best_compression_level` signature `proc get_best_compression_level(data: [i32]) -> i32 {` example `stats.get_best_compression_level(...)` stability `stable`
- `proc get_best_strategy` signature `proc get_best_strategy(data: [i32]) -> i32 {` example `stats.get_best_strategy(...)` stability `stable`
- `proc estimate_best_algorithm` signature `proc estimate_best_algorithm(data: [i32]) -> i32 {` example `stats.estimate_best_algorithm(...)` stability `stable`
- `form CompressionStats` signature `form CompressionStats {` example `stats.CompressionStats` stability `stable`
- `form CompressionStatsManifest` signature `form CompressionStatsManifest {` example `stats.CompressionStatsManifest` stability `stable`
- `form CompressionStatsHealth` signature `form CompressionStatsHealth {` example `stats.CompressionStatsHealth` stability `stable`
- `form CompressionStatsSummary` signature `form CompressionStatsSummary {` example `stats.CompressionStatsSummary` stability `stable`
- `proc analyze_data` signature `proc analyze_data(data: [i32]) -> CompressionStats {` example `stats.analyze_data(...)` stability `stable`
- `proc compress_ex` signature `proc compress_ex(data: [i32], algo: i32, level: i32, strategy: i32) -> [i32] {` example `stats.compress_ex(...)` stability `stable`
- `proc compress_auto` signature `proc compress_auto(data: [i32]) -> [i32] {` example `stats.compress_auto(...)` stability `stable`
- `proc compression_stats_version` signature `proc compression_stats_version() -> string {` example `stats.compression_stats_version(...)` stability `stable`
- `proc compression_stats_ready` signature `proc compression_stats_ready() -> bool {` example `stats.compression_stats_ready(...)` stability `stable`
- `proc compression_stats_manifest` signature `proc compression_stats_manifest() -> CompressionStatsManifest {` example `stats.compression_stats_manifest(...)` stability `stable`
- `proc compression_stats_health` signature `proc compression_stats_health() -> CompressionStatsHealth {` example `stats.compression_stats_health(...)` stability `stable`
- `proc compression_stats_summary` signature `proc compression_stats_summary() -> CompressionStatsSummary {` example `stats.compression_stats_summary(...)` stability `stable`
- `proc compression_stats_selftest` signature `proc compression_stats_selftest() -> bool {` example `stats.compression_stats_selftest(...)` stability `stable`

## `src/vitte/stdlib/crypto/hash.vitl`

Stability: `stable`

- `const HASH_MD5` signature `const HASH_MD5: i32 = 1` example `hash.HASH_MD5` stability `stable`
- `const HASH_SHA1` signature `const HASH_SHA1: i32 = 2` example `hash.HASH_SHA1` stability `stable`
- `const HASH_SHA256` signature `const HASH_SHA256: i32 = 3` example `hash.HASH_SHA256` stability `stable`
- `const HASH_SHA512` signature `const HASH_SHA512: i32 = 4` example `hash.HASH_SHA512` stability `stable`
- `const HASH_SHA3_256` signature `const HASH_SHA3_256: i32 = 5` example `hash.HASH_SHA3_256` stability `stable`
- `const HASH_SHA3_512` signature `const HASH_SHA3_512: i32 = 6` example `hash.HASH_SHA3_512` stability `stable`
- `const HASH_BLAKE2B` signature `const HASH_BLAKE2B: i32 = 7` example `hash.HASH_BLAKE2B` stability `stable`
- `const MD5_DIGEST_SIZE` signature `const MD5_DIGEST_SIZE: i32 = 16` example `hash.MD5_DIGEST_SIZE` stability `stable`
- `const SHA1_DIGEST_SIZE` signature `const SHA1_DIGEST_SIZE: i32 = 20` example `hash.SHA1_DIGEST_SIZE` stability `stable`
- `const SHA256_DIGEST_SIZE` signature `const SHA256_DIGEST_SIZE: i32 = 32` example `hash.SHA256_DIGEST_SIZE` stability `stable`
- `const SHA512_DIGEST_SIZE` signature `const SHA512_DIGEST_SIZE: i32 = 64` example `hash.SHA512_DIGEST_SIZE` stability `stable`
- `const AES_ECB` signature `const AES_ECB: i32 = 1` example `hash.AES_ECB` stability `stable`
- `const AES_CBC` signature `const AES_CBC: i32 = 2` example `hash.AES_CBC` stability `stable`
- `const AES_CTR` signature `const AES_CTR: i32 = 3` example `hash.AES_CTR` stability `stable`
- `const AES_GCM` signature `const AES_GCM: i32 = 4` example `hash.AES_GCM` stability `stable`
- `const AES_128` signature `const AES_128: i32 = 16` example `hash.AES_128` stability `stable`
- `const AES_192` signature `const AES_192: i32 = 24` example `hash.AES_192` stability `stable`
- `const AES_256` signature `const AES_256: i32 = 32` example `hash.AES_256` stability `stable`
- `form Hash` signature `form Hash {` example `hash.Hash` stability `stable`
- `form HashManifest` signature `form HashManifest {` example `hash.HashManifest` stability `stable`
- `proc md5` signature `proc md5(data: string) -> string {` example `hash.md5(...)` stability `stable`
- `proc md5_hex` signature `proc md5_hex(data: string) -> string {` example `hash.md5_hex(...)` stability `stable`
- `proc sha1` signature `proc sha1(data: string) -> string {` example `hash.sha1(...)` stability `stable`
- `proc sha1_hex` signature `proc sha1_hex(data: string) -> string {` example `hash.sha1_hex(...)` stability `stable`
- `proc sha256` signature `proc sha256(data: string) -> string {` example `hash.sha256(...)` stability `stable`
- `proc sha256_hex` signature `proc sha256_hex(data: string) -> string {` example `hash.sha256_hex(...)` stability `stable`
- `proc sha512` signature `proc sha512(data: string) -> string {` example `hash.sha512(...)` stability `stable`
- `proc sha512_hex` signature `proc sha512_hex(data: string) -> string {` example `hash.sha512_hex(...)` stability `stable`
- `proc sha3_256` signature `proc sha3_256(data: string) -> string {` example `hash.sha3_256(...)` stability `stable`
- `proc sha3_256_hex` signature `proc sha3_256_hex(data: string) -> string {` example `hash.sha3_256_hex(...)` stability `stable`
- `proc sha3_512` signature `proc sha3_512(data: string) -> string {` example `hash.sha3_512(...)` stability `stable`
- `proc sha3_512_hex` signature `proc sha3_512_hex(data: string) -> string {` example `hash.sha3_512_hex(...)` stability `stable`
- `proc blake2b` signature `proc blake2b(data: string, size: i32) -> string {` example `hash.blake2b(...)` stability `stable`
- `proc blake2b_hex` signature `proc blake2b_hex(data: string, size: i32) -> string {` example `hash.blake2b_hex(...)` stability `stable`
- `proc hash_new` signature `proc hash_new(algorithm: i32) -> Hash {` example `hash.hash_new(...)` stability `stable`
- `proc hash_update` signature `proc hash_update(h: Hash, data: string) -> int {` example `hash.hash_update(...)` stability `stable`
- `proc hash_final` signature `proc hash_final(h: Hash) -> string {` example `hash.hash_final(...)` stability `stable`
- `proc hash_final_hex` signature `proc hash_final_hex(h: Hash) -> string {` example `hash.hash_final_hex(...)` stability `stable`
- `proc hash_version` signature `proc hash_version() -> string {` example `hash.hash_version(...)` stability `stable`
- `proc hash_ready` signature `proc hash_ready() -> bool {` example `hash.hash_ready(...)` stability `stable`
- `proc hash_manifest` signature `proc hash_manifest() -> HashManifest {` example `hash.hash_manifest(...)` stability `stable`
- `proc hash_selftest` signature `proc hash_selftest() -> bool {` example `hash.hash_selftest(...)` stability `stable`

## `src/vitte/stdlib/crypto/keyderivation.vitl`

Stability: `stable`

- `form KeyDerivationManifest` signature `form KeyDerivationManifest {` example `keyderivation.KeyDerivationManifest` stability `stable`
- `proc pbkdf2` signature `proc pbkdf2(password: string, salt: string, iterations: i32, length: i32) -> string {` example `keyderivation.pbkdf2(...)` stability `stable`
- `proc bcrypt_hash` signature `proc bcrypt_hash(password: string, rounds: i32) -> string {` example `keyderivation.bcrypt_hash(...)` stability `stable`
- `proc bcrypt_verify` signature `proc bcrypt_verify(password: string, hash: string) -> int {` example `keyderivation.bcrypt_verify(...)` stability `stable`
- `proc keyderivation_version` signature `proc keyderivation_version() -> string {` example `keyderivation.keyderivation_version(...)` stability `stable`
- `proc keyderivation_ready` signature `proc keyderivation_ready() -> bool {` example `keyderivation.keyderivation_ready(...)` stability `stable`
- `proc keyderivation_manifest` signature `proc keyderivation_manifest() -> KeyDerivationManifest {` example `keyderivation.keyderivation_manifest(...)` stability `stable`
- `proc keyderivation_selftest` signature `proc keyderivation_selftest() -> bool {` example `keyderivation.keyderivation_selftest(...)` stability `stable`

## `src/vitte/stdlib/crypto/utils.vitl`

Stability: `stable`

- `form CryptoUtilsManifest` signature `form CryptoUtilsManifest {` example `utils.CryptoUtilsManifest` stability `stable`
- `proc hash_compare` signature `proc hash_compare(hash1: string, hash2: string) -> int {` example `utils.hash_compare(...)` stability `stable`
- `proc hash_file` signature `proc hash_file(filepath: string, algorithm: i32) -> string {` example `utils.hash_file(...)` stability `stable`
- `proc hash_file_hex` signature `proc hash_file_hex(filepath: string, algorithm: i32) -> string {` example `utils.hash_file_hex(...)` stability `stable`
- `proc crypt_encode_base64` signature `proc crypt_encode_base64(data: string) -> string {` example `utils.crypt_encode_base64(...)` stability `stable`
- `proc crypt_decode_base64` signature `proc crypt_decode_base64(data: string) -> string {` example `utils.crypt_decode_base64(...)` stability `stable`
- `proc crypt_encode_hex` signature `proc crypt_encode_hex(data: string) -> string {` example `utils.crypt_encode_hex(...)` stability `stable`
- `proc crypt_decode_hex` signature `proc crypt_decode_hex(data: string) -> string {` example `utils.crypt_decode_hex(...)` stability `stable`
- `proc crypto_utils_version` signature `proc crypto_utils_version() -> string {` example `utils.crypto_utils_version(...)` stability `stable`
- `proc crypto_utils_ready` signature `proc crypto_utils_ready() -> bool {` example `utils.crypto_utils_ready(...)` stability `stable`
- `proc crypto_utils_manifest` signature `proc crypto_utils_manifest() -> CryptoUtilsManifest {` example `utils.crypto_utils_manifest(...)` stability `stable`
- `proc crypto_utils_selftest` signature `proc crypto_utils_selftest() -> bool {` example `utils.crypto_utils_selftest(...)` stability `stable`

## `src/vitte/stdlib/encoding/utf.vitl`

Stability: `stable`

- `proc utf8_encode` signature `proc utf8_encode(code_point: i32) -> string {` example `utf.utf8_encode(...)` stability `stable`
- `proc utf8_decode` signature `proc utf8_decode(encoded: string) -> i32 {` example `utf.utf8_decode(...)` stability `stable`
- `proc utf8_is_valid` signature `proc utf8_is_valid(text_value: string) -> int {` example `utf.utf8_is_valid(...)` stability `stable`
- `proc utf8_length` signature `proc utf8_length(text_value: string) -> i32 {` example `utf.utf8_length(...)` stability `stable`
- `proc utf8_byte_length` signature `proc utf8_byte_length(text_value: string) -> i32 {` example `utf.utf8_byte_length(...)` stability `stable`
- `proc utf8_char_at` signature `proc utf8_char_at(text_value: string, index: i32) -> i32 {` example `utf.utf8_char_at(...)` stability `stable`
- `proc utf8_substring` signature `proc utf8_substring(text_value: string, start: i32, end: i32) -> string {` example `utf.utf8_substring(...)` stability `stable`
- `proc utf16_encode` signature `proc utf16_encode(code_point: i32) -> string {` example `utf.utf16_encode(...)` stability `stable`
- `proc utf16_decode` signature `proc utf16_decode(encoded: string) -> i32 {` example `utf.utf16_decode(...)` stability `stable`
- `proc utf16_to_utf8` signature `proc utf16_to_utf8(text_value: string) -> string {` example `utf.utf16_to_utf8(...)` stability `stable`
- `proc utf8_to_utf16` signature `proc utf8_to_utf16(text_value: string) -> string {` example `utf.utf8_to_utf16(...)` stability `stable`
- `proc utf32_encode` signature `proc utf32_encode(code_point: i32) -> string {` example `utf.utf32_encode(...)` stability `stable`
- `proc utf32_decode` signature `proc utf32_decode(encoded: string) -> i32 {` example `utf.utf32_decode(...)` stability `stable`

## `src/vitte/stdlib/examples/stdlib_max.vit`

Stability: `stable`

- `proc stdlib_max_example` signature `proc stdlib_max_example() -> bool {` example `stdlib_max.stdlib_max_example(...)` stability `stable`

## `src/vitte/stdlib/examples/stdlib_usage_examples.vit`

Stability: `stable`

- `proc stdlib_usage_examples` signature `proc stdlib_usage_examples() -> bool {` example `stdlib_usage_examples.stdlib_usage_examples(...)` stability `stable`

## `src/vitte/stdlib/examples/public_module_examples.vit`

Stability: `stable`

- `const PUBLIC_MODULE_EXAMPLE_COUNT` signature `const PUBLIC_MODULE_EXAMPLE_COUNT: usize = 153` example `public_module_examples.PUBLIC_MODULE_EXAMPLE_COUNT` stability `stable`
- `const PUBLIC_MODULE_000_EXAMPLE` signature `const PUBLIC_MODULE_000_EXAMPLE: string = "src/vitte/stdlib/mod.vit"` example `public_module_examples.PUBLIC_MODULE_000_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_001_EXAMPLE` signature `const PUBLIC_MODULE_001_EXAMPLE: string = "src/vitte/stdlib/core.vitl"` example `public_module_examples.PUBLIC_MODULE_001_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_002_EXAMPLE` signature `const PUBLIC_MODULE_002_EXAMPLE: string = "src/vitte/stdlib/core_alias.vitl"` example `public_module_examples.PUBLIC_MODULE_002_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_003_EXAMPLE` signature `const PUBLIC_MODULE_003_EXAMPLE: string = "src/vitte/stdlib/memory.vitl"` example `public_module_examples.PUBLIC_MODULE_003_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_004_EXAMPLE` signature `const PUBLIC_MODULE_004_EXAMPLE: string = "src/vitte/stdlib/collections.vitl"` example `public_module_examples.PUBLIC_MODULE_004_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_005_EXAMPLE` signature `const PUBLIC_MODULE_005_EXAMPLE: string = "src/vitte/stdlib/math.vitl"` example `public_module_examples.PUBLIC_MODULE_005_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_006_EXAMPLE` signature `const PUBLIC_MODULE_006_EXAMPLE: string = "src/vitte/stdlib/strings.vitl"` example `public_module_examples.PUBLIC_MODULE_006_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_007_EXAMPLE` signature `const PUBLIC_MODULE_007_EXAMPLE: string = "src/vitte/stdlib/encoding.vitl"` example `public_module_examples.PUBLIC_MODULE_007_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_008_EXAMPLE` signature `const PUBLIC_MODULE_008_EXAMPLE: string = "src/vitte/stdlib/json.vitl"` example `public_module_examples.PUBLIC_MODULE_008_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_009_EXAMPLE` signature `const PUBLIC_MODULE_009_EXAMPLE: string = "src/vitte/stdlib/crypto.vitl"` example `public_module_examples.PUBLIC_MODULE_009_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_010_EXAMPLE` signature `const PUBLIC_MODULE_010_EXAMPLE: string = "src/vitte/stdlib/compression.vitl"` example `public_module_examples.PUBLIC_MODULE_010_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_011_EXAMPLE` signature `const PUBLIC_MODULE_011_EXAMPLE: string = "src/vitte/stdlib/regex.vitl"` example `public_module_examples.PUBLIC_MODULE_011_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_012_EXAMPLE` signature `const PUBLIC_MODULE_012_EXAMPLE: string = "src/vitte/stdlib/io.vitl"` example `public_module_examples.PUBLIC_MODULE_012_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_013_EXAMPLE` signature `const PUBLIC_MODULE_013_EXAMPLE: string = "src/vitte/stdlib/path.vitl"` example `public_module_examples.PUBLIC_MODULE_013_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_014_EXAMPLE` signature `const PUBLIC_MODULE_014_EXAMPLE: string = "src/vitte/stdlib/os.vitl"` example `public_module_examples.PUBLIC_MODULE_014_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_015_EXAMPLE` signature `const PUBLIC_MODULE_015_EXAMPLE: string = "src/vitte/stdlib/sysinfo.vitl"` example `public_module_examples.PUBLIC_MODULE_015_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_016_EXAMPLE` signature `const PUBLIC_MODULE_016_EXAMPLE: string = "src/vitte/stdlib/datetime.vitl"` example `public_module_examples.PUBLIC_MODULE_016_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_017_EXAMPLE` signature `const PUBLIC_MODULE_017_EXAMPLE: string = "src/vitte/stdlib/runtime.vitl"` example `public_module_examples.PUBLIC_MODULE_017_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_018_EXAMPLE` signature `const PUBLIC_MODULE_018_EXAMPLE: string = "src/vitte/stdlib/kernel.vitl"` example `public_module_examples.PUBLIC_MODULE_018_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_019_EXAMPLE` signature `const PUBLIC_MODULE_019_EXAMPLE: string = "src/vitte/stdlib/graphics.vitl"` example `public_module_examples.PUBLIC_MODULE_019_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_020_EXAMPLE` signature `const PUBLIC_MODULE_020_EXAMPLE: string = "src/vitte/stdlib/core/algorithms.vitl"` example `public_module_examples.PUBLIC_MODULE_020_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_021_EXAMPLE` signature `const PUBLIC_MODULE_021_EXAMPLE: string = "src/vitte/stdlib/core/array.vitl"` example `public_module_examples.PUBLIC_MODULE_021_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_022_EXAMPLE` signature `const PUBLIC_MODULE_022_EXAMPLE: string = "src/vitte/stdlib/core/ascii.vitl"` example `public_module_examples.PUBLIC_MODULE_022_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_023_EXAMPLE` signature `const PUBLIC_MODULE_023_EXAMPLE: string = "src/vitte/stdlib/core/clone.vitl"` example `public_module_examples.PUBLIC_MODULE_023_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_024_EXAMPLE` signature `const PUBLIC_MODULE_024_EXAMPLE: string = "src/vitte/stdlib/core/cmp.vitl"` example `public_module_examples.PUBLIC_MODULE_024_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_025_EXAMPLE` signature `const PUBLIC_MODULE_025_EXAMPLE: string = "src/vitte/stdlib/core/concurrency.vitl"` example `public_module_examples.PUBLIC_MODULE_025_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_026_EXAMPLE` signature `const PUBLIC_MODULE_026_EXAMPLE: string = "src/vitte/stdlib/core/convert.vitl"` example `public_module_examples.PUBLIC_MODULE_026_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_027_EXAMPLE` signature `const PUBLIC_MODULE_027_EXAMPLE: string = "src/vitte/stdlib/core/default.vitl"` example `public_module_examples.PUBLIC_MODULE_027_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_028_EXAMPLE` signature `const PUBLIC_MODULE_028_EXAMPLE: string = "src/vitte/stdlib/core/drop.vitl"` example `public_module_examples.PUBLIC_MODULE_028_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_029_EXAMPLE` signature `const PUBLIC_MODULE_029_EXAMPLE: string = "src/vitte/stdlib/core/float.vitl"` example `public_module_examples.PUBLIC_MODULE_029_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_030_EXAMPLE` signature `const PUBLIC_MODULE_030_EXAMPLE: string = "src/vitte/stdlib/core/hash.vitl"` example `public_module_examples.PUBLIC_MODULE_030_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_031_EXAMPLE` signature `const PUBLIC_MODULE_031_EXAMPLE: string = "src/vitte/stdlib/core/io_helpers.vitl"` example `public_module_examples.PUBLIC_MODULE_031_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_032_EXAMPLE` signature `const PUBLIC_MODULE_032_EXAMPLE: string = "src/vitte/stdlib/core/iterator.vitl"` example `public_module_examples.PUBLIC_MODULE_032_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_033_EXAMPLE` signature `const PUBLIC_MODULE_033_EXAMPLE: string = "src/vitte/stdlib/core/math.vitl"` example `public_module_examples.PUBLIC_MODULE_033_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_034_EXAMPLE` signature `const PUBLIC_MODULE_034_EXAMPLE: string = "src/vitte/stdlib/core/memory.vitl"` example `public_module_examples.PUBLIC_MODULE_034_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_035_EXAMPLE` signature `const PUBLIC_MODULE_035_EXAMPLE: string = "src/vitte/stdlib/core/number.vitl"` example `public_module_examples.PUBLIC_MODULE_035_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_036_EXAMPLE` signature `const PUBLIC_MODULE_036_EXAMPLE: string = "src/vitte/stdlib/core/panic.vitl"` example `public_module_examples.PUBLIC_MODULE_036_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_037_EXAMPLE` signature `const PUBLIC_MODULE_037_EXAMPLE: string = "src/vitte/stdlib/core/option.vitl"` example `public_module_examples.PUBLIC_MODULE_037_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_038_EXAMPLE` signature `const PUBLIC_MODULE_038_EXAMPLE: string = "src/vitte/stdlib/core/primitive.vitl"` example `public_module_examples.PUBLIC_MODULE_038_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_039_EXAMPLE` signature `const PUBLIC_MODULE_039_EXAMPLE: string = "src/vitte/stdlib/core/range.vitl"` example `public_module_examples.PUBLIC_MODULE_039_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_040_EXAMPLE` signature `const PUBLIC_MODULE_040_EXAMPLE: string = "src/vitte/stdlib/core/result.vitl"` example `public_module_examples.PUBLIC_MODULE_040_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_041_EXAMPLE` signature `const PUBLIC_MODULE_041_EXAMPLE: string = "src/vitte/stdlib/core/scope.vitl"` example `public_module_examples.PUBLIC_MODULE_041_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_042_EXAMPLE` signature `const PUBLIC_MODULE_042_EXAMPLE: string = "src/vitte/stdlib/core/slice.vitl"` example `public_module_examples.PUBLIC_MODULE_042_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_043_EXAMPLE` signature `const PUBLIC_MODULE_043_EXAMPLE: string = "src/vitte/stdlib/core/string.vitl"` example `public_module_examples.PUBLIC_MODULE_043_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_044_EXAMPLE` signature `const PUBLIC_MODULE_044_EXAMPLE: string = "src/vitte/stdlib/core/strings.vitl"` example `public_module_examples.PUBLIC_MODULE_044_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_045_EXAMPLE` signature `const PUBLIC_MODULE_045_EXAMPLE: string = "src/vitte/stdlib/core/types.vitl"` example `public_module_examples.PUBLIC_MODULE_045_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_046_EXAMPLE` signature `const PUBLIC_MODULE_046_EXAMPLE: string = "src/vitte/stdlib/core/unicode.vitl"` example `public_module_examples.PUBLIC_MODULE_046_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_047_EXAMPLE` signature `const PUBLIC_MODULE_047_EXAMPLE: string = "src/vitte/stdlib/core/utils.vitl"` example `public_module_examples.PUBLIC_MODULE_047_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_048_EXAMPLE` signature `const PUBLIC_MODULE_048_EXAMPLE: string = "src/vitte/stdlib/alloc/arc.vitl"` example `public_module_examples.PUBLIC_MODULE_048_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_049_EXAMPLE` signature `const PUBLIC_MODULE_049_EXAMPLE: string = "src/vitte/stdlib/alloc/box.vitl"` example `public_module_examples.PUBLIC_MODULE_049_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_050_EXAMPLE` signature `const PUBLIC_MODULE_050_EXAMPLE: string = "src/vitte/stdlib/alloc/collections.vitl"` example `public_module_examples.PUBLIC_MODULE_050_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_051_EXAMPLE` signature `const PUBLIC_MODULE_051_EXAMPLE: string = "src/vitte/stdlib/alloc/rc.vitl"` example `public_module_examples.PUBLIC_MODULE_051_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_052_EXAMPLE` signature `const PUBLIC_MODULE_052_EXAMPLE: string = "src/vitte/stdlib/alloc/string.vitl"` example `public_module_examples.PUBLIC_MODULE_052_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_053_EXAMPLE` signature `const PUBLIC_MODULE_053_EXAMPLE: string = "src/vitte/stdlib/alloc/vec.vitl"` example `public_module_examples.PUBLIC_MODULE_053_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_054_EXAMPLE` signature `const PUBLIC_MODULE_054_EXAMPLE: string = "src/vitte/stdlib/async/async.vitl"` example `public_module_examples.PUBLIC_MODULE_054_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_055_EXAMPLE` signature `const PUBLIC_MODULE_055_EXAMPLE: string = "src/vitte/stdlib/async/channel.vitl"` example `public_module_examples.PUBLIC_MODULE_055_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_056_EXAMPLE` signature `const PUBLIC_MODULE_056_EXAMPLE: string = "src/vitte/stdlib/async/future.vitl"` example `public_module_examples.PUBLIC_MODULE_056_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_057_EXAMPLE` signature `const PUBLIC_MODULE_057_EXAMPLE: string = "src/vitte/stdlib/collections/queue.vitl"` example `public_module_examples.PUBLIC_MODULE_057_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_058_EXAMPLE` signature `const PUBLIC_MODULE_058_EXAMPLE: string = "src/vitte/stdlib/collections/vector.vitl"` example `public_module_examples.PUBLIC_MODULE_058_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_059_EXAMPLE` signature `const PUBLIC_MODULE_059_EXAMPLE: string = "src/vitte/stdlib/compression/algorithms.vitl"` example `public_module_examples.PUBLIC_MODULE_059_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_060_EXAMPLE` signature `const PUBLIC_MODULE_060_EXAMPLE: string = "src/vitte/stdlib/compression/brotli.vitl"` example `public_module_examples.PUBLIC_MODULE_060_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_061_EXAMPLE` signature `const PUBLIC_MODULE_061_EXAMPLE: string = "src/vitte/stdlib/compression/deflate.vitl"` example `public_module_examples.PUBLIC_MODULE_061_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_062_EXAMPLE` signature `const PUBLIC_MODULE_062_EXAMPLE: string = "src/vitte/stdlib/compression/huffman.vitl"` example `public_module_examples.PUBLIC_MODULE_062_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_063_EXAMPLE` signature `const PUBLIC_MODULE_063_EXAMPLE: string = "src/vitte/stdlib/compression/interface.vitl"` example `public_module_examples.PUBLIC_MODULE_063_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_064_EXAMPLE` signature `const PUBLIC_MODULE_064_EXAMPLE: string = "src/vitte/stdlib/compression/lz.vitl"` example `public_module_examples.PUBLIC_MODULE_064_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_065_EXAMPLE` signature `const PUBLIC_MODULE_065_EXAMPLE: string = "src/vitte/stdlib/compression/stats.vitl"` example `public_module_examples.PUBLIC_MODULE_065_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_066_EXAMPLE` signature `const PUBLIC_MODULE_066_EXAMPLE: string = "src/vitte/stdlib/crypto/hash.vitl"` example `public_module_examples.PUBLIC_MODULE_066_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_067_EXAMPLE` signature `const PUBLIC_MODULE_067_EXAMPLE: string = "src/vitte/stdlib/crypto/keyderivation.vitl"` example `public_module_examples.PUBLIC_MODULE_067_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_068_EXAMPLE` signature `const PUBLIC_MODULE_068_EXAMPLE: string = "src/vitte/stdlib/crypto/utils.vitl"` example `public_module_examples.PUBLIC_MODULE_068_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_069_EXAMPLE` signature `const PUBLIC_MODULE_069_EXAMPLE: string = "src/vitte/stdlib/encoding/utf.vitl"` example `public_module_examples.PUBLIC_MODULE_069_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_070_EXAMPLE` signature `const PUBLIC_MODULE_070_EXAMPLE: string = "src/vitte/stdlib/json/builder.vitl"` example `public_module_examples.PUBLIC_MODULE_070_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_071_EXAMPLE` signature `const PUBLIC_MODULE_071_EXAMPLE: string = "src/vitte/stdlib/json/parse.vitl"` example `public_module_examples.PUBLIC_MODULE_071_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_072_EXAMPLE` signature `const PUBLIC_MODULE_072_EXAMPLE: string = "src/vitte/stdlib/json/stringify.vitl"` example `public_module_examples.PUBLIC_MODULE_072_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_073_EXAMPLE` signature `const PUBLIC_MODULE_073_EXAMPLE: string = "src/vitte/stdlib/json/types.vitl"` example `public_module_examples.PUBLIC_MODULE_073_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_074_EXAMPLE` signature `const PUBLIC_MODULE_074_EXAMPLE: string = "src/vitte/stdlib/kernel/fileio.vitl"` example `public_module_examples.PUBLIC_MODULE_074_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_075_EXAMPLE` signature `const PUBLIC_MODULE_075_EXAMPLE: string = "src/vitte/stdlib/kernel/memory.vitl"` example `public_module_examples.PUBLIC_MODULE_075_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_076_EXAMPLE` signature `const PUBLIC_MODULE_076_EXAMPLE: string = "src/vitte/stdlib/kernel/sync.vitl"` example `public_module_examples.PUBLIC_MODULE_076_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_077_EXAMPLE` signature `const PUBLIC_MODULE_077_EXAMPLE: string = "src/vitte/stdlib/kernel/users.vitl"` example `public_module_examples.PUBLIC_MODULE_077_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_078_EXAMPLE` signature `const PUBLIC_MODULE_078_EXAMPLE: string = "src/vitte/stdlib/math/algebra.vitl"` example `public_module_examples.PUBLIC_MODULE_078_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_079_EXAMPLE` signature `const PUBLIC_MODULE_079_EXAMPLE: string = "src/vitte/stdlib/math/arithmetic.vitl"` example `public_module_examples.PUBLIC_MODULE_079_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_080_EXAMPLE` signature `const PUBLIC_MODULE_080_EXAMPLE: string = "src/vitte/stdlib/math/arrays.vitl"` example `public_module_examples.PUBLIC_MODULE_080_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_081_EXAMPLE` signature `const PUBLIC_MODULE_081_EXAMPLE: string = "src/vitte/stdlib/math/calculus.vitl"` example `public_module_examples.PUBLIC_MODULE_081_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_082_EXAMPLE` signature `const PUBLIC_MODULE_082_EXAMPLE: string = "src/vitte/stdlib/math/comparison.vitl"` example `public_module_examples.PUBLIC_MODULE_082_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_083_EXAMPLE` signature `const PUBLIC_MODULE_083_EXAMPLE: string = "src/vitte/stdlib/math/complex.vitl"` example `public_module_examples.PUBLIC_MODULE_083_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_084_EXAMPLE` signature `const PUBLIC_MODULE_084_EXAMPLE: string = "src/vitte/stdlib/math/geometry.vitl"` example `public_module_examples.PUBLIC_MODULE_084_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_085_EXAMPLE` signature `const PUBLIC_MODULE_085_EXAMPLE: string = "src/vitte/stdlib/math/matrix.vitl"` example `public_module_examples.PUBLIC_MODULE_085_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_086_EXAMPLE` signature `const PUBLIC_MODULE_086_EXAMPLE: string = "src/vitte/stdlib/math/modular.vitl"` example `public_module_examples.PUBLIC_MODULE_086_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_087_EXAMPLE` signature `const PUBLIC_MODULE_087_EXAMPLE: string = "src/vitte/stdlib/math/number_theory.vitl"` example `public_module_examples.PUBLIC_MODULE_087_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_088_EXAMPLE` signature `const PUBLIC_MODULE_088_EXAMPLE: string = "src/vitte/stdlib/math/powers.vitl"` example `public_module_examples.PUBLIC_MODULE_088_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_089_EXAMPLE` signature `const PUBLIC_MODULE_089_EXAMPLE: string = "src/vitte/stdlib/math/probability.vitl"` example `public_module_examples.PUBLIC_MODULE_089_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_090_EXAMPLE` signature `const PUBLIC_MODULE_090_EXAMPLE: string = "src/vitte/stdlib/math/roots.vitl"` example `public_module_examples.PUBLIC_MODULE_090_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_091_EXAMPLE` signature `const PUBLIC_MODULE_091_EXAMPLE: string = "src/vitte/stdlib/math/sequences.vitl"` example `public_module_examples.PUBLIC_MODULE_091_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_092_EXAMPLE` signature `const PUBLIC_MODULE_092_EXAMPLE: string = "src/vitte/stdlib/math/sort.vitl"` example `public_module_examples.PUBLIC_MODULE_092_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_093_EXAMPLE` signature `const PUBLIC_MODULE_093_EXAMPLE: string = "src/vitte/stdlib/math/statistics.vitl"` example `public_module_examples.PUBLIC_MODULE_093_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_094_EXAMPLE` signature `const PUBLIC_MODULE_094_EXAMPLE: string = "src/vitte/stdlib/math/topology.vitl"` example `public_module_examples.PUBLIC_MODULE_094_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_095_EXAMPLE` signature `const PUBLIC_MODULE_095_EXAMPLE: string = "src/vitte/stdlib/math/trigonometry.vitl"` example `public_module_examples.PUBLIC_MODULE_095_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_096_EXAMPLE` signature `const PUBLIC_MODULE_096_EXAMPLE: string = "src/vitte/stdlib/math/vector.vitl"` example `public_module_examples.PUBLIC_MODULE_096_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_097_EXAMPLE` signature `const PUBLIC_MODULE_097_EXAMPLE: string = "src/vitte/stdlib/network/http.vitl"` example `public_module_examples.PUBLIC_MODULE_097_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_098_EXAMPLE` signature `const PUBLIC_MODULE_098_EXAMPLE: string = "src/vitte/stdlib/network/socket.vitl"` example `public_module_examples.PUBLIC_MODULE_098_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_099_EXAMPLE` signature `const PUBLIC_MODULE_099_EXAMPLE: string = "src/vitte/stdlib/network/udp.vitl"` example `public_module_examples.PUBLIC_MODULE_099_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_100_EXAMPLE` signature `const PUBLIC_MODULE_100_EXAMPLE: string = "src/vitte/stdlib/path/globbing.vitl"` example `public_module_examples.PUBLIC_MODULE_100_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_101_EXAMPLE` signature `const PUBLIC_MODULE_101_EXAMPLE: string = "src/vitte/stdlib/path/manipulation.vitl"` example `public_module_examples.PUBLIC_MODULE_101_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_102_EXAMPLE` signature `const PUBLIC_MODULE_102_EXAMPLE: string = "src/vitte/stdlib/path/special.vitl"` example `public_module_examples.PUBLIC_MODULE_102_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_103_EXAMPLE` signature `const PUBLIC_MODULE_103_EXAMPLE: string = "src/vitte/stdlib/path/walker.vitl"` example `public_module_examples.PUBLIC_MODULE_103_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_104_EXAMPLE` signature `const PUBLIC_MODULE_104_EXAMPLE: string = "src/vitte/stdlib/platform/abi.vitl"` example `public_module_examples.PUBLIC_MODULE_104_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_105_EXAMPLE` signature `const PUBLIC_MODULE_105_EXAMPLE: string = "src/vitte/stdlib/platform/embedded.vitl"` example `public_module_examples.PUBLIC_MODULE_105_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_106_EXAMPLE` signature `const PUBLIC_MODULE_106_EXAMPLE: string = "src/vitte/stdlib/platform/posix.vitl"` example `public_module_examples.PUBLIC_MODULE_106_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_107_EXAMPLE` signature `const PUBLIC_MODULE_107_EXAMPLE: string = "src/vitte/stdlib/platform/wasm.vitl"` example `public_module_examples.PUBLIC_MODULE_107_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_108_EXAMPLE` signature `const PUBLIC_MODULE_108_EXAMPLE: string = "src/vitte/stdlib/platform/windows.vitl"` example `public_module_examples.PUBLIC_MODULE_108_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_109_EXAMPLE` signature `const PUBLIC_MODULE_109_EXAMPLE: string = "src/vitte/stdlib/profiling/profiler.vitl"` example `public_module_examples.PUBLIC_MODULE_109_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_110_EXAMPLE` signature `const PUBLIC_MODULE_110_EXAMPLE: string = "src/vitte/stdlib/std/atomic.vitl"` example `public_module_examples.PUBLIC_MODULE_110_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_111_EXAMPLE` signature `const PUBLIC_MODULE_111_EXAMPLE: string = "src/vitte/stdlib/std/base64.vitl"` example `public_module_examples.PUBLIC_MODULE_111_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_112_EXAMPLE` signature `const PUBLIC_MODULE_112_EXAMPLE: string = "src/vitte/stdlib/std/bench.vitl"` example `public_module_examples.PUBLIC_MODULE_112_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_113_EXAMPLE` signature `const PUBLIC_MODULE_113_EXAMPLE: string = "src/vitte/stdlib/std/calendar.vitl"` example `public_module_examples.PUBLIC_MODULE_113_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_114_EXAMPLE` signature `const PUBLIC_MODULE_114_EXAMPLE: string = "src/vitte/stdlib/std/cli.vitl"` example `public_module_examples.PUBLIC_MODULE_114_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_115_EXAMPLE` signature `const PUBLIC_MODULE_115_EXAMPLE: string = "src/vitte/stdlib/std/csv.vitl"` example `public_module_examples.PUBLIC_MODULE_115_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_116_EXAMPLE` signature `const PUBLIC_MODULE_116_EXAMPLE: string = "src/vitte/stdlib/std/diff.vitl"` example `public_module_examples.PUBLIC_MODULE_116_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_117_EXAMPLE` signature `const PUBLIC_MODULE_117_EXAMPLE: string = "src/vitte/stdlib/std/env.vitl"` example `public_module_examples.PUBLIC_MODULE_117_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_118_EXAMPLE` signature `const PUBLIC_MODULE_118_EXAMPLE: string = "src/vitte/stdlib/std/error.vitl"` example `public_module_examples.PUBLIC_MODULE_118_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_119_EXAMPLE` signature `const PUBLIC_MODULE_119_EXAMPLE: string = "src/vitte/stdlib/std/event.vitl"` example `public_module_examples.PUBLIC_MODULE_119_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_120_EXAMPLE` signature `const PUBLIC_MODULE_120_EXAMPLE: string = "src/vitte/stdlib/std/format.vitl"` example `public_module_examples.PUBLIC_MODULE_120_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_121_EXAMPLE` signature `const PUBLIC_MODULE_121_EXAMPLE: string = "src/vitte/stdlib/std/fs.vitl"` example `public_module_examples.PUBLIC_MODULE_121_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_122_EXAMPLE` signature `const PUBLIC_MODULE_122_EXAMPLE: string = "src/vitte/stdlib/std/glob.vitl"` example `public_module_examples.PUBLIC_MODULE_122_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_123_EXAMPLE` signature `const PUBLIC_MODULE_123_EXAMPLE: string = "src/vitte/stdlib/std/hash.vitl"` example `public_module_examples.PUBLIC_MODULE_123_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_124_EXAMPLE` signature `const PUBLIC_MODULE_124_EXAMPLE: string = "src/vitte/stdlib/std/http.vitl"` example `public_module_examples.PUBLIC_MODULE_124_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_125_EXAMPLE` signature `const PUBLIC_MODULE_125_EXAMPLE: string = "src/vitte/stdlib/std/io.vitl"` example `public_module_examples.PUBLIC_MODULE_125_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_126_EXAMPLE` signature `const PUBLIC_MODULE_126_EXAMPLE: string = "src/vitte/stdlib/std/kernel.vitl"` example `public_module_examples.PUBLIC_MODULE_126_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_127_EXAMPLE` signature `const PUBLIC_MODULE_127_EXAMPLE: string = "src/vitte/stdlib/std/locale.vitl"` example `public_module_examples.PUBLIC_MODULE_127_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_128_EXAMPLE` signature `const PUBLIC_MODULE_128_EXAMPLE: string = "src/vitte/stdlib/std/log.vitl"` example `public_module_examples.PUBLIC_MODULE_128_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_129_EXAMPLE` signature `const PUBLIC_MODULE_129_EXAMPLE: string = "src/vitte/stdlib/std/metrics.vitl"` example `public_module_examples.PUBLIC_MODULE_129_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_130_EXAMPLE` signature `const PUBLIC_MODULE_130_EXAMPLE: string = "src/vitte/stdlib/std/mime.vitl"` example `public_module_examples.PUBLIC_MODULE_130_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_131_EXAMPLE` signature `const PUBLIC_MODULE_131_EXAMPLE: string = "src/vitte/stdlib/std/net.vitl"` example `public_module_examples.PUBLIC_MODULE_131_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_132_EXAMPLE` signature `const PUBLIC_MODULE_132_EXAMPLE: string = "src/vitte/stdlib/std/parse.vitl"` example `public_module_examples.PUBLIC_MODULE_132_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_133_EXAMPLE` signature `const PUBLIC_MODULE_133_EXAMPLE: string = "src/vitte/stdlib/std/path.vitl"` example `public_module_examples.PUBLIC_MODULE_133_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_134_EXAMPLE` signature `const PUBLIC_MODULE_134_EXAMPLE: string = "src/vitte/stdlib/std/percent_encoding.vitl"` example `public_module_examples.PUBLIC_MODULE_134_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_135_EXAMPLE` signature `const PUBLIC_MODULE_135_EXAMPLE: string = "src/vitte/stdlib/std/process.vitl"` example `public_module_examples.PUBLIC_MODULE_135_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_136_EXAMPLE` signature `const PUBLIC_MODULE_136_EXAMPLE: string = "src/vitte/stdlib/std/random.vitl"` example `public_module_examples.PUBLIC_MODULE_136_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_137_EXAMPLE` signature `const PUBLIC_MODULE_137_EXAMPLE: string = "src/vitte/stdlib/std/serialization.vitl"` example `public_module_examples.PUBLIC_MODULE_137_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_138_EXAMPLE` signature `const PUBLIC_MODULE_138_EXAMPLE: string = "src/vitte/stdlib/std/semver.vitl"` example `public_module_examples.PUBLIC_MODULE_138_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_139_EXAMPLE` signature `const PUBLIC_MODULE_139_EXAMPLE: string = "src/vitte/stdlib/std/signal.vitl"` example `public_module_examples.PUBLIC_MODULE_139_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_140_EXAMPLE` signature `const PUBLIC_MODULE_140_EXAMPLE: string = "src/vitte/stdlib/std/sync.vitl"` example `public_module_examples.PUBLIC_MODULE_140_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_141_EXAMPLE` signature `const PUBLIC_MODULE_141_EXAMPLE: string = "src/vitte/stdlib/std/tempfile.vitl"` example `public_module_examples.PUBLIC_MODULE_141_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_142_EXAMPLE` signature `const PUBLIC_MODULE_142_EXAMPLE: string = "src/vitte/stdlib/std/testing.vitl"` example `public_module_examples.PUBLIC_MODULE_142_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_143_EXAMPLE` signature `const PUBLIC_MODULE_143_EXAMPLE: string = "src/vitte/stdlib/std/thread.vitl"` example `public_module_examples.PUBLIC_MODULE_143_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_144_EXAMPLE` signature `const PUBLIC_MODULE_144_EXAMPLE: string = "src/vitte/stdlib/std/time.vitl"` example `public_module_examples.PUBLIC_MODULE_144_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_145_EXAMPLE` signature `const PUBLIC_MODULE_145_EXAMPLE: string = "src/vitte/stdlib/std/terminal.vitl"` example `public_module_examples.PUBLIC_MODULE_145_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_146_EXAMPLE` signature `const PUBLIC_MODULE_146_EXAMPLE: string = "src/vitte/stdlib/std/units.vitl"` example `public_module_examples.PUBLIC_MODULE_146_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_147_EXAMPLE` signature `const PUBLIC_MODULE_147_EXAMPLE: string = "src/vitte/stdlib/std/uri.vitl"` example `public_module_examples.PUBLIC_MODULE_147_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_148_EXAMPLE` signature `const PUBLIC_MODULE_148_EXAMPLE: string = "src/vitte/stdlib/std/url.vitl"` example `public_module_examples.PUBLIC_MODULE_148_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_149_EXAMPLE` signature `const PUBLIC_MODULE_149_EXAMPLE: string = "src/vitte/stdlib/std/uuid.vitl"` example `public_module_examples.PUBLIC_MODULE_149_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_EXTRA_158_EXAMPLE` signature `const PUBLIC_MODULE_EXTRA_158_EXAMPLE: string = "src/vitte/stdlib/alloc/btree.vitl"` example `public_module_examples.PUBLIC_MODULE_EXTRA_158_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_EXTRA_159_EXAMPLE` signature `const PUBLIC_MODULE_EXTRA_159_EXAMPLE: string = "src/vitte/stdlib/alloc/deque.vitl"` example `public_module_examples.PUBLIC_MODULE_EXTRA_159_EXAMPLE` stability `stable`
- `const PUBLIC_MODULE_EXTRA_160_EXAMPLE` signature `const PUBLIC_MODULE_EXTRA_160_EXAMPLE: string = "src/vitte/stdlib/alloc/smallvec.vitl"` example `public_module_examples.PUBLIC_MODULE_EXTRA_160_EXAMPLE` stability `stable`
- `proc public_module_examples_present` signature `proc public_module_examples_present() -> bool {` example `public_module_examples.public_module_examples_present(...)` stability `stable`

## `src/vitte/stdlib/generated/unicode_tables.vitl`

Stability: `internal`

- `const GENERATED_UNICODE_VERSION` signature `const GENERATED_UNICODE_VERSION: string = "15.1.0"` example `unicode_tables.GENERATED_UNICODE_VERSION` stability `internal`
- `const GENERATED_UNICODE_SOURCE` signature `const GENERATED_UNICODE_SOURCE: string = "Unicode Character Database"` example `unicode_tables.GENERATED_UNICODE_SOURCE` stability `internal`
- `const GENERATED_UNICODE_CHECKSUM` signature `const GENERATED_UNICODE_CHECKSUM: string = "sha256:3ef91cd09d2e67d51ca928349eeb2c5176cb551c85e8f23228944b8a30141f9b"` example `unicode_tables.GENERATED_UNICODE_CHECKSUM` stability `internal`
- `proc generated_unicode_version` signature `proc generated_unicode_version() -> string { give GENERATED_UNICODE_VERSION; }` example `unicode_tables.generated_unicode_version(...)` stability `internal`
- `proc generated_unicode_source` signature `proc generated_unicode_source() -> string { give GENERATED_UNICODE_SOURCE; }` example `unicode_tables.generated_unicode_source(...)` stability `internal`
- `proc generated_unicode_checksum` signature `proc generated_unicode_checksum() -> string { give GENERATED_UNICODE_CHECKSUM; }` example `unicode_tables.generated_unicode_checksum(...)` stability `internal`
- `proc generated_unicode_category` signature `proc generated_unicode_category(value: rune) -> UnicodeCategory { give compiler_generated_unicode_category(value); }` example `unicode_tables.generated_unicode_category(...)` stability `internal`
- `proc generated_unicode_properties` signature `proc generated_unicode_properties(value: rune) -> UnicodeProperties { give compiler_generated_unicode_properties(value); }` example `unicode_tables.generated_unicode_properties(...)` stability `internal`
- `proc generated_unicode_normalization` signature `proc generated_unicode_normalization(form: NormalizationForm, value: rune) -> Iterator<rune> { give compiler_generated_unicode_normalization(form, value); }` example `unicode_tables.generated_unicode_normalization(...)` stability `internal`
- `proc generated_unicode_case_fold` signature `proc generated_unicode_case_fold(value: rune) -> Iterator<rune> { give compiler_generated_unicode_case_fold(value); }` example `unicode_tables.generated_unicode_case_fold(...)` stability `internal`

## `src/vitte/stdlib/json/builder.vitl`

Stability: `stable`

- `pick JSONValue` signature `pick JSONValue {` example `builder.JSONValue` stability `stable`
- `form JSONBuilder` signature `form JSONBuilder {` example `builder.JSONBuilder` stability `stable`
- `form JsonBuilderManifest` signature `form JsonBuilderManifest {` example `builder.JsonBuilderManifest` stability `stable`
- `proc json_builder_new` signature `proc json_builder_new() -> JSONBuilder {` example `builder.json_builder_new(...)` stability `stable`
- `proc json_builder_append_null` signature `proc json_builder_append_null(b: JSONBuilder) -> int {` example `builder.json_builder_append_null(...)` stability `stable`
- `proc json_builder_append_bool` signature `proc json_builder_append_bool(b: JSONBuilder, value: int) -> int {` example `builder.json_builder_append_bool(...)` stability `stable`
- `proc json_builder_append_number` signature `proc json_builder_append_number(b: JSONBuilder, value: f64) -> int {` example `builder.json_builder_append_number(...)` stability `stable`
- `proc json_builder_append_string` signature `proc json_builder_append_string(b: JSONBuilder, value: string) -> int {` example `builder.json_builder_append_string(...)` stability `stable`
- `proc json_builder_start_object` signature `proc json_builder_start_object(b: JSONBuilder) -> int {` example `builder.json_builder_start_object(...)` stability `stable`
- `proc json_builder_end_object` signature `proc json_builder_end_object(b: JSONBuilder) -> int {` example `builder.json_builder_end_object(...)` stability `stable`
- `proc json_builder_start_array` signature `proc json_builder_start_array(b: JSONBuilder) -> int {` example `builder.json_builder_start_array(...)` stability `stable`
- `proc json_builder_end_array` signature `proc json_builder_end_array(b: JSONBuilder) -> int {` example `builder.json_builder_end_array(...)` stability `stable`
- `proc json_builder_append_comma` signature `proc json_builder_append_comma(b: JSONBuilder) -> int {` example `builder.json_builder_append_comma(...)` stability `stable`
- `proc json_builder_append_colon` signature `proc json_builder_append_colon(b: JSONBuilder) -> int {` example `builder.json_builder_append_colon(...)` stability `stable`
- `proc json_builder_to_string` signature `proc json_builder_to_string(b: JSONBuilder) -> string {` example `builder.json_builder_to_string(...)` stability `stable`
- `proc json_builder_clear` signature `proc json_builder_clear(b: JSONBuilder) {` example `builder.json_builder_clear(...)` stability `stable`
- `proc json_builder_version` signature `proc json_builder_version() -> string {` example `builder.json_builder_version(...)` stability `stable`
- `proc json_builder_ready` signature `proc json_builder_ready() -> bool {` example `builder.json_builder_ready(...)` stability `stable`
- `proc json_builder_manifest` signature `proc json_builder_manifest() -> JsonBuilderManifest {` example `builder.json_builder_manifest(...)` stability `stable`
- `proc json_builder_selftest` signature `proc json_builder_selftest() -> bool {` example `builder.json_builder_selftest(...)` stability `stable`

## `src/vitte/stdlib/json/parse.vitl`

Stability: `stable`

- `pick JSONValue` signature `pick JSONValue {` example `parse.JSONValue` stability `stable`
- `form JSONParser` signature `form JSONParser {` example `parse.JSONParser` stability `stable`
- `form JsonParseManifest` signature `form JsonParseManifest {` example `parse.JsonParseManifest` stability `stable`
- `proc json_parse` signature `proc json_parse(text_value: string) -> JSONValue {` example `parse.json_parse(...)` stability `stable`
- `proc json_parse_object` signature `proc json_parse_object(parser: JSONParser) -> [string] {` example `parse.json_parse_object(...)` stability `stable`
- `proc json_parse_array` signature `proc json_parse_array(parser: JSONParser) -> [JSONValue] {` example `parse.json_parse_array(...)` stability `stable`
- `proc json_parse_string` signature `proc json_parse_string(parser: JSONParser) -> string {` example `parse.json_parse_string(...)` stability `stable`
- `proc json_parse_number` signature `proc json_parse_number(parser: JSONParser) -> f64 {` example `parse.json_parse_number(...)` stability `stable`
- `proc json_parse_bool` signature `proc json_parse_bool(parser: JSONParser) -> int {` example `parse.json_parse_bool(...)` stability `stable`
- `proc json_parse_null` signature `proc json_parse_null(parser: JSONParser) -> int {` example `parse.json_parse_null(...)` stability `stable`
- `proc json_parse_version` signature `proc json_parse_version() -> string {` example `parse.json_parse_version(...)` stability `stable`
- `proc json_parse_ready` signature `proc json_parse_ready() -> bool {` example `parse.json_parse_ready(...)` stability `stable`
- `proc json_parse_manifest` signature `proc json_parse_manifest() -> JsonParseManifest {` example `parse.json_parse_manifest(...)` stability `stable`
- `proc json_parse_selftest` signature `proc json_parse_selftest() -> bool {` example `parse.json_parse_selftest(...)` stability `stable`

## `src/vitte/stdlib/json/stringify.vitl`

Stability: `stable`

- `pick JSONValue` signature `pick JSONValue {` example `stringify.JSONValue` stability `stable`
- `form JsonStringifyManifest` signature `form JsonStringifyManifest {` example `stringify.JsonStringifyManifest` stability `stable`
- `proc json_stringify` signature `proc json_stringify(value: JSONValue) -> string {` example `stringify.json_stringify(...)` stability `stable`
- `proc json_stringify_pretty` signature `proc json_stringify_pretty(value: JSONValue, indent: i32) -> string {` example `stringify.json_stringify_pretty(...)` stability `stable`
- `proc json_is_valid` signature `proc json_is_valid(text_value: string) -> int {` example `stringify.json_is_valid(...)` stability `stable`
- `proc json_format` signature `proc json_format(text_value: string) -> string {` example `stringify.json_format(...)` stability `stable`
- `proc json_minify` signature `proc json_minify(text_value: string) -> string {` example `stringify.json_minify(...)` stability `stable`
- `proc json_value_type` signature `proc json_value_type(value: JSONValue) -> string {` example `stringify.json_value_type(...)` stability `stable`
- `proc json_stringify_version` signature `proc json_stringify_version() -> string {` example `stringify.json_stringify_version(...)` stability `stable`
- `proc json_stringify_ready` signature `proc json_stringify_ready() -> bool {` example `stringify.json_stringify_ready(...)` stability `stable`
- `proc json_stringify_manifest` signature `proc json_stringify_manifest() -> JsonStringifyManifest {` example `stringify.json_stringify_manifest(...)` stability `stable`
- `proc json_stringify_selftest` signature `proc json_stringify_selftest() -> bool {` example `stringify.json_stringify_selftest(...)` stability `stable`

## `src/vitte/stdlib/json/types.vitl`

Stability: `stable`

- `pick JSONValue` signature `pick JSONValue {` example `types.JSONValue` stability `stable`
- `form JSONParser` signature `form JSONParser {` example `types.JSONParser` stability `stable`
- `form JSONBuilder` signature `form JSONBuilder {` example `types.JSONBuilder` stability `stable`
- `form JsonTypesManifest` signature `form JsonTypesManifest {` example `types.JsonTypesManifest` stability `stable`
- `proc json_types_version` signature `proc json_types_version() -> string {` example `types.json_types_version(...)` stability `stable`
- `proc json_types_ready` signature `proc json_types_ready() -> bool {` example `types.json_types_ready(...)` stability `stable`
- `proc json_types_manifest` signature `proc json_types_manifest() -> JsonTypesManifest {` example `types.json_types_manifest(...)` stability `stable`
- `proc json_types_selftest` signature `proc json_types_selftest() -> bool {` example `types.json_types_selftest(...)` stability `stable`

## `src/vitte/stdlib/kernel/fileio.vitl`

Stability: `stable`

- `const STDIN_FILENO` signature `const STDIN_FILENO: i32 = 0` example `fileio.STDIN_FILENO` stability `stable`
- `const STDOUT_FILENO` signature `const STDOUT_FILENO: i32 = 1` example `fileio.STDOUT_FILENO` stability `stable`
- `const STDERR_FILENO` signature `const STDERR_FILENO: i32 = 2` example `fileio.STDERR_FILENO` stability `stable`
- `const O_RDONLY` signature `const O_RDONLY: i32 = 0` example `fileio.O_RDONLY` stability `stable`
- `const O_WRONLY` signature `const O_WRONLY: i32 = 1` example `fileio.O_WRONLY` stability `stable`
- `const O_RDWR` signature `const O_RDWR: i32 = 2` example `fileio.O_RDWR` stability `stable`
- `const O_APPEND` signature `const O_APPEND: i32 = 1024` example `fileio.O_APPEND` stability `stable`
- `const O_CREAT` signature `const O_CREAT: i32 = 64` example `fileio.O_CREAT` stability `stable`
- `const O_EXCL` signature `const O_EXCL: i32 = 128` example `fileio.O_EXCL` stability `stable`
- `const O_TRUNC` signature `const O_TRUNC: i32 = 512` example `fileio.O_TRUNC` stability `stable`
- `const O_NONBLOCK` signature `const O_NONBLOCK: i32 = 2048` example `fileio.O_NONBLOCK` stability `stable`
- `const O_SYNC` signature `const O_SYNC: i32 = 4096` example `fileio.O_SYNC` stability `stable`
- `const S_IRUSR` signature `const S_IRUSR: i32 = 256` example `fileio.S_IRUSR` stability `stable`
- `const S_IWUSR` signature `const S_IWUSR: i32 = 128` example `fileio.S_IWUSR` stability `stable`
- `const S_IXUSR` signature `const S_IXUSR: i32 = 64` example `fileio.S_IXUSR` stability `stable`
- `const S_IRGRP` signature `const S_IRGRP: i32 = 32` example `fileio.S_IRGRP` stability `stable`
- `const S_IWGRP` signature `const S_IWGRP: i32 = 16` example `fileio.S_IWGRP` stability `stable`
- `const S_IXGRP` signature `const S_IXGRP: i32 = 8` example `fileio.S_IXGRP` stability `stable`
- `const S_IROTH` signature `const S_IROTH: i32 = 4` example `fileio.S_IROTH` stability `stable`
- `const S_IWOTH` signature `const S_IWOTH: i32 = 2` example `fileio.S_IWOTH` stability `stable`
- `const S_IXOTH` signature `const S_IXOTH: i32 = 1` example `fileio.S_IXOTH` stability `stable`
- `const S_ISREG` signature `const S_ISREG: i32 = 32768` example `fileio.S_ISREG` stability `stable`
- `const S_ISDIR` signature `const S_ISDIR: i32 = 16384` example `fileio.S_ISDIR` stability `stable`
- `const S_ISCHR` signature `const S_ISCHR: i32 = 8192` example `fileio.S_ISCHR` stability `stable`
- `const S_ISBLK` signature `const S_ISBLK: i32 = 24576` example `fileio.S_ISBLK` stability `stable`
- `const S_ISFIFO` signature `const S_ISFIFO: i32 = 4096` example `fileio.S_ISFIFO` stability `stable`
- `const S_ISLNK` signature `const S_ISLNK: i32 = 40960` example `fileio.S_ISLNK` stability `stable`
- `const S_ISSOCK` signature `const S_ISSOCK: i32 = 49152` example `fileio.S_ISSOCK` stability `stable`
- `const SEEK_SET` signature `const SEEK_SET: i32 = 0` example `fileio.SEEK_SET` stability `stable`
- `const SEEK_CUR` signature `const SEEK_CUR: i32 = 1` example `fileio.SEEK_CUR` stability `stable`
- `const SEEK_END` signature `const SEEK_END: i32 = 2` example `fileio.SEEK_END` stability `stable`
- `form DirEnt` signature `form DirEnt {` example `fileio.DirEnt` stability `stable`
- `form FileStat` signature `form FileStat {` example `fileio.FileStat` stability `stable`
- `proc open` signature `proc open(path: string, flag_bits: i32, mode: i32) -> i32 {` example `fileio.open(...)` stability `stable`
- `proc creat` signature `proc creat(path: string, mode: i32) -> i32 {` example `fileio.creat(...)` stability `stable`
- `proc close` signature `proc close(fd: i32) -> int {` example `fileio.close(...)` stability `stable`
- `proc read` signature `proc read(fd: i32, nbytes: i64) -> string {` example `fileio.read(...)` stability `stable`
- `proc write` signature `proc write(fd: i32, buf: string) -> i64 {` example `fileio.write(...)` stability `stable`
- `proc lseek` signature `proc lseek(fd: i32, offset: i64, whence: i32) -> i64 {` example `fileio.lseek(...)` stability `stable`
- `proc pread` signature `proc pread(fd: i32, offset: i64, nbytes: i64) -> string {` example `fileio.pread(...)` stability `stable`
- `proc pwrite` signature `proc pwrite(fd: i32, buf: string, offset: i64) -> i64 {` example `fileio.pwrite(...)` stability `stable`
- `proc fsync` signature `proc fsync(fd: i32) -> int {` example `fileio.fsync(...)` stability `stable`
- `proc fdatasync` signature `proc fdatasync(fd: i32) -> int {` example `fileio.fdatasync(...)` stability `stable`
- `proc dup` signature `proc dup(fd: i32) -> i32 {` example `fileio.dup(...)` stability `stable`
- `proc dup2` signature `proc dup2(oldfd: i32, newfd: i32) -> i32 {` example `fileio.dup2(...)` stability `stable`
- `proc fcntl` signature `proc fcntl(fd: i32, cmd: i32, arg: i32) -> int {` example `fileio.fcntl(...)` stability `stable`
- `proc ioctl` signature `proc ioctl(fd: i32, request: i32, arg: i32) -> int {` example `fileio.ioctl(...)` stability `stable`
- `proc ftruncate` signature `proc ftruncate(fd: i32, length: i64) -> int {` example `fileio.ftruncate(...)` stability `stable`
- `proc flock` signature `proc flock(fd: i32, operation: i32) -> int {` example `fileio.flock(...)` stability `stable`
- `proc mkdir` signature `proc mkdir(path: string, mode: i32) -> int {` example `fileio.mkdir(...)` stability `stable`
- `proc rmdir` signature `proc rmdir(path: string) -> int {` example `fileio.rmdir(...)` stability `stable`
- `proc opendir` signature `proc opendir(path: string) -> i32 {` example `fileio.opendir(...)` stability `stable`
- `proc closedir` signature `proc closedir(dirfd: i32) -> int {` example `fileio.closedir(...)` stability `stable`
- `proc readdir` signature `proc readdir(dirfd: i32) -> DirEnt {` example `fileio.readdir(...)` stability `stable`
- `proc rewinddir` signature `proc rewinddir(dirfd: i32) {` example `fileio.rewinddir(...)` stability `stable`
- `proc chdir` signature `proc chdir(path: string) -> int {` example `fileio.chdir(...)` stability `stable`
- `proc getcwd` signature `proc getcwd() -> string {` example `fileio.getcwd(...)` stability `stable`
- `proc listdir` signature `proc listdir(path: string) -> [string] {` example `fileio.listdir(...)` stability `stable`
- `proc unlink` signature `proc unlink(path: string) -> int {` example `fileio.unlink(...)` stability `stable`
- `proc link` signature `proc link(oldpath: string, newpath: string) -> int {` example `fileio.link(...)` stability `stable`
- `proc symlink` signature `proc symlink(oldpath: string, newpath: string) -> int {` example `fileio.symlink(...)` stability `stable`
- `proc readlink` signature `proc readlink(path: string) -> string {` example `fileio.readlink(...)` stability `stable`
- `proc rename` signature `proc rename(oldpath: string, newpath: string) -> int {` example `fileio.rename(...)` stability `stable`
- `proc chmod` signature `proc chmod(path: string, mode: i32) -> int {` example `fileio.chmod(...)` stability `stable`
- `proc fchmod` signature `proc fchmod(fd: i32, mode: i32) -> int {` example `fileio.fchmod(...)` stability `stable`
- `proc chown` signature `proc chown(path: string, uid: i32, gid: i32) -> int {` example `fileio.chown(...)` stability `stable`
- `proc fchown` signature `proc fchown(fd: i32, uid: i32, gid: i32) -> int {` example `fileio.fchown(...)` stability `stable`
- `proc stat` signature `proc stat(path: string) -> FileStat {` example `fileio.stat(...)` stability `stable`
- `proc fstat` signature `proc fstat(fd: i32) -> FileStat {` example `fileio.fstat(...)` stability `stable`
- `proc lstat` signature `proc lstat(path: string) -> FileStat {` example `fileio.lstat(...)` stability `stable`
- `proc access` signature `proc access(path: string, mode: i32) -> int {` example `fileio.access(...)` stability `stable`
- `proc truncate` signature `proc truncate(path: string, length: i64) -> int {` example `fileio.truncate(...)` stability `stable`
- `proc statvfs` signature `proc statvfs(path: string) -> [i64] {` example `fileio.statvfs(...)` stability `stable`
- `proc pathconf` signature `proc pathconf(path: string, name: string) -> i64 {` example `fileio.pathconf(...)` stability `stable`
- `proc fpathconf` signature `proc fpathconf(fd: i32, name: string) -> i64 {` example `fileio.fpathconf(...)` stability `stable`
- `proc sync` signature `proc sync() {` example `fileio.sync(...)` stability `stable`
- `proc umask` signature `proc umask(mask: i32) -> i32 {` example `fileio.umask(...)` stability `stable`

## `src/vitte/stdlib/kernel/memory.vitl`

Stability: `stable`

- `const PROT_NONE` signature `const PROT_NONE: i32 = 0` example `memory.PROT_NONE` stability `stable`
- `const PROT_READ` signature `const PROT_READ: i32 = 1` example `memory.PROT_READ` stability `stable`
- `const PROT_WRITE` signature `const PROT_WRITE: i32 = 2` example `memory.PROT_WRITE` stability `stable`
- `const PROT_EXEC` signature `const PROT_EXEC: i32 = 4` example `memory.PROT_EXEC` stability `stable`
- `const MAP_SHARED` signature `const MAP_SHARED: i32 = 1` example `memory.MAP_SHARED` stability `stable`
- `const MAP_PRIVATE` signature `const MAP_PRIVATE: i32 = 2` example `memory.MAP_PRIVATE` stability `stable`
- `const MAP_FIXED` signature `const MAP_FIXED: i32 = 10` example `memory.MAP_FIXED` stability `stable`
- `const MAP_ANONYMOUS` signature `const MAP_ANONYMOUS: i32 = 32` example `memory.MAP_ANONYMOUS` stability `stable`
- `const MADV_NORMAL` signature `const MADV_NORMAL: i32 = 0` example `memory.MADV_NORMAL` stability `stable`
- `const MADV_RANDOM` signature `const MADV_RANDOM: i32 = 1` example `memory.MADV_RANDOM` stability `stable`
- `const MADV_SEQUENTIAL` signature `const MADV_SEQUENTIAL: i32 = 2` example `memory.MADV_SEQUENTIAL` stability `stable`
- `const MADV_WILLNEED` signature `const MADV_WILLNEED: i32 = 3` example `memory.MADV_WILLNEED` stability `stable`
- `const MADV_DONTNEED` signature `const MADV_DONTNEED: i32 = 4` example `memory.MADV_DONTNEED` stability `stable`
- `const MCL_CURRENT` signature `const MCL_CURRENT: i32 = 1` example `memory.MCL_CURRENT` stability `stable`
- `const MCL_FUTURE` signature `const MCL_FUTURE: i32 = 2` example `memory.MCL_FUTURE` stability `stable`
- `const PAGE_SIZE` signature `const PAGE_SIZE: i64 = 4096` example `memory.PAGE_SIZE` stability `stable`
- `proc mmap` signature `proc mmap(addr: i64, length: i64, prot: i32, flag_bits: i32, fd: i32, offset: i64) -> i64 {` example `memory.mmap(...)` stability `stable`
- `proc munmap` signature `proc munmap(addr: i64, length: i64) -> int {` example `memory.munmap(...)` stability `stable`
- `proc mprotect` signature `proc mprotect(addr: i64, length: i64, prot: i32) -> int {` example `memory.mprotect(...)` stability `stable`
- `proc madvise` signature `proc madvise(addr: i64, length: i64, advice: i32) -> int {` example `memory.madvise(...)` stability `stable`
- `proc msync` signature `proc msync(addr: i64, length: i64, flag_bits: i32) -> int {` example `memory.msync(...)` stability `stable`
- `proc mlock` signature `proc mlock(addr: i64, length: i64) -> int {` example `memory.mlock(...)` stability `stable`
- `proc munlock` signature `proc munlock(addr: i64, length: i64) -> int {` example `memory.munlock(...)` stability `stable`
- `proc mlockall` signature `proc mlockall(flag_bits: i32) -> int {` example `memory.mlockall(...)` stability `stable`
- `proc munlockall` signature `proc munlockall() -> int {` example `memory.munlockall(...)` stability `stable`
- `proc brk` signature `proc brk(addr: i64) -> i64 {` example `memory.brk(...)` stability `stable`
- `proc sbrk` signature `proc sbrk(increment: i64) -> i64 {` example `memory.sbrk(...)` stability `stable`
- `form MemInfo` signature `form MemInfo {` example `memory.MemInfo` stability `stable`
- `proc get_meminfo` signature `proc get_meminfo() -> MemInfo {` example `memory.get_meminfo(...)` stability `stable`
- `proc get_page_size` signature `proc get_page_size() -> i64 {` example `memory.get_page_size(...)` stability `stable`
- `proc get_phys_pages` signature `proc get_phys_pages() -> i64 {` example `memory.get_phys_pages(...)` stability `stable`
- `proc get_avphys_pages` signature `proc get_avphys_pages() -> i64 {` example `memory.get_avphys_pages(...)` stability `stable`
- `proc mreconcile` signature `proc mreconcile(pages_to_free: i64) -> int {` example `memory.mreconcile(...)` stability `stable`
- `proc mincore` signature `proc mincore(addr: i64, length: i64, vec: [int]) -> int {` example `memory.mincore(...)` stability `stable`
- `proc get_memory_map` signature `proc get_memory_map() -> string {` example `memory.get_memory_map(...)` stability `stable`

## `src/vitte/stdlib/kernel/sync.vitl`

Stability: `stable`

- `form Mutex` signature `form Mutex {` example `sync.Mutex` stability `stable`
- `form Semaphore` signature `form Semaphore {` example `sync.Semaphore` stability `stable`
- `form Lock` signature `form Lock {` example `sync.Lock` stability `stable`
- `form CondVar` signature `form CondVar {` example `sync.CondVar` stability `stable`
- `form Barrier` signature `form Barrier {` example `sync.Barrier` stability `stable`
- `form RWLock` signature `form RWLock {` example `sync.RWLock` stability `stable`
- `const PTHREAD_MUTEX_NORMAL` signature `const PTHREAD_MUTEX_NORMAL: i32 = 0` example `sync.PTHREAD_MUTEX_NORMAL` stability `stable`
- `const PTHREAD_MUTEX_RECURSIVE` signature `const PTHREAD_MUTEX_RECURSIVE: i32 = 1` example `sync.PTHREAD_MUTEX_RECURSIVE` stability `stable`
- `const PTHREAD_MUTEX_ERRORCHECK` signature `const PTHREAD_MUTEX_ERRORCHECK: i32 = 2` example `sync.PTHREAD_MUTEX_ERRORCHECK` stability `stable`
- `proc pthread_mutex_create` signature `proc pthread_mutex_create() -> Mutex {` example `sync.pthread_mutex_create(...)` stability `stable`
- `proc pthread_mutex_lock` signature `proc pthread_mutex_lock(mutex: Mutex) -> int {` example `sync.pthread_mutex_lock(...)` stability `stable`
- `proc pthread_mutex_unlock` signature `proc pthread_mutex_unlock(mutex: Mutex) -> int {` example `sync.pthread_mutex_unlock(...)` stability `stable`
- `proc pthread_mutex_trylock` signature `proc pthread_mutex_trylock(mutex: Mutex) -> int {` example `sync.pthread_mutex_trylock(...)` stability `stable`
- `proc pthread_mutex_timedlock` signature `proc pthread_mutex_timedlock(mutex: Mutex, timeout_ms: i64) -> int {` example `sync.pthread_mutex_timedlock(...)` stability `stable`
- `proc pthread_mutex_destroy` signature `proc pthread_mutex_destroy(mutex: Mutex) -> int {` example `sync.pthread_mutex_destroy(...)` stability `stable`
- `proc sem_create` signature `proc sem_create(initial_value: int) -> Semaphore {` example `sync.sem_create(...)` stability `stable`
- `proc sem_wait` signature `proc sem_wait(sem: Semaphore) -> int {` example `sync.sem_wait(...)` stability `stable`
- `proc sem_trywait` signature `proc sem_trywait(sem: Semaphore) -> int {` example `sync.sem_trywait(...)` stability `stable`
- `proc sem_timedwait` signature `proc sem_timedwait(sem: Semaphore, timeout_ms: i64) -> int {` example `sync.sem_timedwait(...)` stability `stable`
- `proc sem_post` signature `proc sem_post(sem: Semaphore) -> int {` example `sync.sem_post(...)` stability `stable`
- `proc sem_getvalue` signature `proc sem_getvalue(sem: Semaphore) -> int {` example `sync.sem_getvalue(...)` stability `stable`
- `proc sem_destroy` signature `proc sem_destroy(sem: Semaphore) -> int {` example `sync.sem_destroy(...)` stability `stable`
- `proc pthread_cond_create` signature `proc pthread_cond_create() -> CondVar {` example `sync.pthread_cond_create(...)` stability `stable`
- `proc pthread_cond_wait` signature `proc pthread_cond_wait(cond: CondVar, mutex: Mutex) -> int {` example `sync.pthread_cond_wait(...)` stability `stable`
- `proc pthread_cond_timedwait` signature `proc pthread_cond_timedwait(cond: CondVar, mutex: Mutex, timeout_ms: i64) -> int {` example `sync.pthread_cond_timedwait(...)` stability `stable`
- `proc pthread_cond_signal` signature `proc pthread_cond_signal(cond: CondVar) -> int {` example `sync.pthread_cond_signal(...)` stability `stable`
- `proc pthread_cond_broadcast` signature `proc pthread_cond_broadcast(cond: CondVar) -> int {` example `sync.pthread_cond_broadcast(...)` stability `stable`
- `proc pthread_cond_destroy` signature `proc pthread_cond_destroy(cond: CondVar) -> int {` example `sync.pthread_cond_destroy(...)` stability `stable`
- `proc pthread_barrier_create` signature `proc pthread_barrier_create(count: int) -> Barrier {` example `sync.pthread_barrier_create(...)` stability `stable`
- `proc pthread_barrier_wait` signature `proc pthread_barrier_wait(barrier: Barrier) -> int {` example `sync.pthread_barrier_wait(...)` stability `stable`
- `proc pthread_barrier_destroy` signature `proc pthread_barrier_destroy(barrier: Barrier) -> int {` example `sync.pthread_barrier_destroy(...)` stability `stable`
- `proc pthread_rwlock_create` signature `proc pthread_rwlock_create() -> RWLock {` example `sync.pthread_rwlock_create(...)` stability `stable`
- `proc pthread_rwlock_rdlock` signature `proc pthread_rwlock_rdlock(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_rdlock(...)` stability `stable`
- `proc pthread_rwlock_wrlock` signature `proc pthread_rwlock_wrlock(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_wrlock(...)` stability `stable`
- `proc pthread_rwlock_tryrdlock` signature `proc pthread_rwlock_tryrdlock(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_tryrdlock(...)` stability `stable`
- `proc pthread_rwlock_trywrlock` signature `proc pthread_rwlock_trywrlock(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_trywrlock(...)` stability `stable`
- `proc pthread_rwlock_unlock` signature `proc pthread_rwlock_unlock(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_unlock(...)` stability `stable`
- `proc pthread_rwlock_destroy` signature `proc pthread_rwlock_destroy(rwlock: RWLock) -> int {` example `sync.pthread_rwlock_destroy(...)` stability `stable`
- `proc pthread_spin_create` signature `proc pthread_spin_create() -> Lock {` example `sync.pthread_spin_create(...)` stability `stable`
- `proc pthread_spin_lock` signature `proc pthread_spin_lock(spinlock: Lock) -> int {` example `sync.pthread_spin_lock(...)` stability `stable`
- `proc pthread_spin_unlock` signature `proc pthread_spin_unlock(spinlock: Lock) -> int {` example `sync.pthread_spin_unlock(...)` stability `stable`
- `proc pthread_spin_trylock` signature `proc pthread_spin_trylock(spinlock: Lock) -> int {` example `sync.pthread_spin_trylock(...)` stability `stable`
- `proc pthread_spin_destroy` signature `proc pthread_spin_destroy(spinlock: Lock) -> int {` example `sync.pthread_spin_destroy(...)` stability `stable`

## `src/vitte/stdlib/kernel/users.vitl`

Stability: `stable`

- `const RLIMIT_CPU` signature `const RLIMIT_CPU: i32 = 0` example `users.RLIMIT_CPU` stability `stable`
- `const RLIMIT_FSIZE` signature `const RLIMIT_FSIZE: i32 = 1` example `users.RLIMIT_FSIZE` stability `stable`
- `const RLIMIT_DATA` signature `const RLIMIT_DATA: i32 = 2` example `users.RLIMIT_DATA` stability `stable`
- `const RLIMIT_STACK` signature `const RLIMIT_STACK: i32 = 3` example `users.RLIMIT_STACK` stability `stable`
- `const RLIMIT_CORE` signature `const RLIMIT_CORE: i32 = 4` example `users.RLIMIT_CORE` stability `stable`
- `const RLIMIT_RSS` signature `const RLIMIT_RSS: i32 = 5` example `users.RLIMIT_RSS` stability `stable`
- `const RLIMIT_NPROC` signature `const RLIMIT_NPROC: i32 = 6` example `users.RLIMIT_NPROC` stability `stable`
- `const RLIMIT_NOFILE` signature `const RLIMIT_NOFILE: i32 = 7` example `users.RLIMIT_NOFILE` stability `stable`
- `const RLIMIT_MEMLOCK` signature `const RLIMIT_MEMLOCK: i32 = 8` example `users.RLIMIT_MEMLOCK` stability `stable`
- `const RLIMIT_AS` signature `const RLIMIT_AS: i32 = 9` example `users.RLIMIT_AS` stability `stable`
- `const RLIM_INFINITY` signature `const RLIM_INFINITY: i64 = 9223372036854775807` example `users.RLIM_INFINITY` stability `stable`
- `form PasswdEntry` signature `form PasswdEntry {` example `users.PasswdEntry` stability `stable`
- `form GroupEntry` signature `form GroupEntry {` example `users.GroupEntry` stability `stable`
- `form RlimitEntry` signature `form RlimitEntry {` example `users.RlimitEntry` stability `stable`
- `proc getuid` signature `proc getuid() -> i32 {` example `users.getuid(...)` stability `stable`
- `proc geteuid` signature `proc geteuid() -> i32 {` example `users.geteuid(...)` stability `stable`
- `proc setuid` signature `proc setuid(uid: i32) -> int {` example `users.setuid(...)` stability `stable`
- `proc seteuid` signature `proc seteuid(uid: i32) -> int {` example `users.seteuid(...)` stability `stable`
- `proc getgid` signature `proc getgid() -> i32 {` example `users.getgid(...)` stability `stable`
- `proc getegid` signature `proc getegid() -> i32 {` example `users.getegid(...)` stability `stable`
- `proc setgid` signature `proc setgid(gid: i32) -> int {` example `users.setgid(...)` stability `stable`
- `proc setegid` signature `proc setegid(gid: i32) -> int {` example `users.setegid(...)` stability `stable`
- `proc getgroups` signature `proc getgroups() -> [i32] {` example `users.getgroups(...)` stability `stable`
- `proc setgroups` signature `proc setgroups(groups: [i32]) -> int {` example `users.setgroups(...)` stability `stable`
- `proc getpid` signature `proc getpid() -> i32 {` example `users.getpid(...)` stability `stable`
- `proc getppid` signature `proc getppid() -> i32 {` example `users.getppid(...)` stability `stable`
- `proc getpgid` signature `proc getpgid(pid: i32) -> i32 {` example `users.getpgid(...)` stability `stable`
- `proc setpgid` signature `proc setpgid(pid: i32, pgid: i32) -> int {` example `users.setpgid(...)` stability `stable`
- `proc getpgrp` signature `proc getpgrp() -> i32 {` example `users.getpgrp(...)` stability `stable`
- `proc setsid` signature `proc setsid() -> i32 {` example `users.setsid(...)` stability `stable`
- `proc getsid` signature `proc getsid(pid: i32) -> i32 {` example `users.getsid(...)` stability `stable`
- `proc getpwuid` signature `proc getpwuid(uid: i32) -> PasswdEntry {` example `users.getpwuid(...)` stability `stable`
- `proc getpwnam` signature `proc getpwnam(name: string) -> PasswdEntry {` example `users.getpwnam(...)` stability `stable`
- `proc getgrgid` signature `proc getgrgid(gid: i32) -> GroupEntry {` example `users.getgrgid(...)` stability `stable`
- `proc getgrnam` signature `proc getgrnam(name: string) -> GroupEntry {` example `users.getgrnam(...)` stability `stable`
- `proc getrlimit` signature `proc getrlimit(resource_id: i32) -> RlimitEntry {` example `users.getrlimit(...)` stability `stable`
- `proc setrlimit` signature `proc setrlimit(resource_id: i32, rlim: RlimitEntry) -> int {` example `users.setrlimit(...)` stability `stable`
- `proc getrlimit_cpu` signature `proc getrlimit_cpu() -> RlimitEntry {` example `users.getrlimit_cpu(...)` stability `stable`
- `proc setrlimit_cpu` signature `proc setrlimit_cpu(limit: RlimitEntry) -> int {` example `users.setrlimit_cpu(...)` stability `stable`
- `proc getrlimit_fsize` signature `proc getrlimit_fsize() -> RlimitEntry {` example `users.getrlimit_fsize(...)` stability `stable`
- `proc setrlimit_fsize` signature `proc setrlimit_fsize(limit: RlimitEntry) -> int {` example `users.setrlimit_fsize(...)` stability `stable`
- `proc getrlimit_nofile` signature `proc getrlimit_nofile() -> RlimitEntry {` example `users.getrlimit_nofile(...)` stability `stable`
- `proc setrlimit_nofile` signature `proc setrlimit_nofile(limit: RlimitEntry) -> int {` example `users.setrlimit_nofile(...)` stability `stable`
- `proc getenv` signature `proc getenv(name: string) -> string {` example `users.getenv(...)` stability `stable`
- `proc setenv` signature `proc setenv(name: string, value: string, overwrite: int) -> int {` example `users.setenv(...)` stability `stable`
- `proc unsetenv` signature `proc unsetenv(name: string) -> int {` example `users.unsetenv(...)` stability `stable`
- `proc environ` signature `proc environ() -> [string] {` example `users.environ(...)` stability `stable`
- `proc clearenv` signature `proc clearenv() -> int {` example `users.clearenv(...)` stability `stable`
- `proc getpriority` signature `proc getpriority(which: i32, who: i32) -> i32 {` example `users.getpriority(...)` stability `stable`
- `proc setpriority` signature `proc setpriority(which: i32, who: i32, prio: i32) -> int {` example `users.setpriority(...)` stability `stable`
- `proc getrusage` signature `proc getrusage(who: i32) -> [i64] {` example `users.getrusage(...)` stability `stable`
- `proc get_login` signature `proc get_login() -> string {` example `users.get_login(...)` stability `stable`
- `proc get_username` signature `proc get_username() -> string {` example `users.get_username(...)` stability `stable`
- `proc get_home_dir` signature `proc get_home_dir() -> string {` example `users.get_home_dir(...)` stability `stable`
- `proc get_shell` signature `proc get_shell() -> string {` example `users.get_shell(...)` stability `stable`
- `proc is_root` signature `proc is_root() -> int {` example `users.is_root(...)` stability `stable`
- `proc access` signature `proc access(pathname: string, mode: i32) -> int {` example `users.access(...)` stability `stable`
- `proc faccessat` signature `proc faccessat(dirfd: i32, pathname: string, mode: i32, flag_bits: i32) -> int {` example `users.faccessat(...)` stability `stable`
- `proc eaccess` signature `proc eaccess(pathname: string, mode: i32) -> int {` example `users.eaccess(...)` stability `stable`
- `proc initgroups` signature `proc initgroups(user_name: string, group: i32) -> int {` example `users.initgroups(...)` stability `stable`

## `src/vitte/stdlib/math/algebra.vitl`

Stability: `stable`

- `proc add_vectors` signature `proc add_vectors(a: [int], b: [int]) -> [int] {` example `algebra.add_vectors(...)` stability `stable`
- `proc dot` signature `proc dot(a: [int], b: [int]) -> int {` example `algebra.dot(...)` stability `stable`
- `proc cross_2d` signature `proc cross_2d(a: [int], b: [int]) -> int {` example `algebra.cross_2d(...)` stability `stable`
- `proc cross_3d` signature `proc cross_3d(a: [int], b: [int]) -> [int] {` example `algebra.cross_3d(...)` stability `stable`
- `proc scalar_multiply` signature `proc scalar_multiply(v: [int], k: int) -> [int] {` example `algebra.scalar_multiply(...)` stability `stable`
- `proc vector_magnitude` signature `proc vector_magnitude(v: [int]) -> int {` example `algebra.vector_magnitude(...)` stability `stable`
- `proc normalize_vector` signature `proc normalize_vector(v: [int]) -> [int] {` example `algebra.normalize_vector(...)` stability `stable`
- `proc matrix_multiply` signature `proc matrix_multiply(a: [[int]], b: [[int]]) -> [[int]] {` example `algebra.matrix_multiply(...)` stability `stable`
- `proc matrix_transpose` signature `proc matrix_transpose(m: [[int]]) -> [[int]] {` example `algebra.matrix_transpose(...)` stability `stable`
- `proc determinant_2x2` signature `proc determinant_2x2(m: [[int]]) -> int {` example `algebra.determinant_2x2(...)` stability `stable`
- `proc solve_linear_system` signature `proc solve_linear_system(a: [[int]], b: [int]) -> [int] {` example `algebra.solve_linear_system(...)` stability `stable`
- `proc algebra_version` signature `proc algebra_version() -> string {` example `algebra.algebra_version(...)` stability `stable`
- `proc algebra_ready` signature `proc algebra_ready() -> bool {` example `algebra.algebra_ready(...)` stability `stable`
- `proc algebra_selftest` signature `proc algebra_selftest() -> bool {` example `algebra.algebra_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/arithmetic.vitl`

Stability: `stable`

- `const PI` signature `const PI: f64 = 3.141592653589793` example `arithmetic.PI` stability `stable`
- `const E` signature `const E: f64 = 2.718281828459045` example `arithmetic.E` stability `stable`
- `const I64_MIN` signature `const I64_MIN: i64 = 9223372036854775808` example `arithmetic.I64_MIN` stability `stable`
- `const I64_MAX` signature `const I64_MAX: i64 = 9223372036854775807` example `arithmetic.I64_MAX` stability `stable`
- `const F64_INF` signature `const F64_INF: f64 = 1000000000000000000.0` example `arithmetic.F64_INF` stability `stable`
- `const F64_NAN` signature `const F64_NAN: f64 = 0.0` example `arithmetic.F64_NAN` stability `stable`
- `const RAND_A` signature `const RAND_A: i64 = 1664525` example `arithmetic.RAND_A` stability `stable`
- `const RAND_C` signature `const RAND_C: i64 = 1013904223` example `arithmetic.RAND_C` stability `stable`
- `const RAND_M` signature `const RAND_M: i64 = 2147483647` example `arithmetic.RAND_M` stability `stable`
- `pick SafeResult` signature `pick SafeResult {` example `arithmetic.SafeResult` stability `stable`
- `form I128` signature `form I128 {` example `arithmetic.I128` stability `stable`
- `proc abs_i64` signature `proc abs_i64(value: i64) -> i64 {` example `arithmetic.abs_i64(...)` stability `stable`
- `proc abs_f64` signature `proc abs_f64(value: f64) -> f64 {` example `arithmetic.abs_f64(...)` stability `stable`
- `proc add_i64` signature `proc add_i64(a: i64, b: i64) -> i64 {` example `arithmetic.add_i64(...)` stability `stable`
- `proc sub_i64` signature `proc sub_i64(a: i64, b: i64) -> i64 {` example `arithmetic.sub_i64(...)` stability `stable`
- `proc mul_i64` signature `proc mul_i64(a: i64, b: i64) -> i64 {` example `arithmetic.mul_i64(...)` stability `stable`
- `proc div_i64` signature `proc div_i64(a: i64, b: i64) -> i64 {` example `arithmetic.div_i64(...)` stability `stable`
- `proc mod_i64` signature `proc mod_i64(a: i64, b: i64) -> i64 {` example `arithmetic.mod_i64(...)` stability `stable`
- `proc add_f64` signature `proc add_f64(a: f64, b: f64) -> f64 {` example `arithmetic.add_f64(...)` stability `stable`
- `proc sub_f64` signature `proc sub_f64(a: f64, b: f64) -> f64 {` example `arithmetic.sub_f64(...)` stability `stable`
- `proc mul_f64` signature `proc mul_f64(a: f64, b: f64) -> f64 {` example `arithmetic.mul_f64(...)` stability `stable`
- `proc div_f64` signature `proc div_f64(a: f64, b: f64) -> f64 {` example `arithmetic.div_f64(...)` stability `stable`
- `proc max_i64` signature `proc max_i64(a: i64, b: i64) -> i64 {` example `arithmetic.max_i64(...)` stability `stable`
- `proc min_i64` signature `proc min_i64(a: i64, b: i64) -> i64 {` example `arithmetic.min_i64(...)` stability `stable`
- `proc max_f64` signature `proc max_f64(a: f64, b: f64) -> f64 {` example `arithmetic.max_f64(...)` stability `stable`
- `proc min_f64` signature `proc min_f64(a: f64, b: f64) -> f64 {` example `arithmetic.min_f64(...)` stability `stable`
- `proc pow_i64` signature `proc pow_i64(base: i64, exponent: i64) -> i64 {` example `arithmetic.pow_i64(...)` stability `stable`
- `proc pow_f64` signature `proc pow_f64(base: f64, exponent: i64) -> f64 {` example `arithmetic.pow_f64(...)` stability `stable`
- `proc fact` signature `proc fact(value: i64) -> i64 {` example `arithmetic.fact(...)` stability `stable`
- `proc gcd` signature `proc gcd(a: i64, b: i64) -> i64 {` example `arithmetic.gcd(...)` stability `stable`
- `proc lcm` signature `proc lcm(a: i64, b: i64) -> i64 {` example `arithmetic.lcm(...)` stability `stable`
- `proc is_prime` signature `proc is_prime(value: i64) -> bool {` example `arithmetic.is_prime(...)` stability `stable`
- `proc fib` signature `proc fib(index: i64) -> i64 {` example `arithmetic.fib(...)` stability `stable`
- `proc clamp_i64` signature `proc clamp_i64(value: i64, low: i64, high: i64) -> i64 {` example `arithmetic.clamp_i64(...)` stability `stable`
- `proc lerp` signature `proc lerp(a: f64, b: f64, t: f64) -> f64 {` example `arithmetic.lerp(...)` stability `stable`
- `proc safe_add` signature `proc safe_add(a: i64, b: i64) -> SafeResult {` example `arithmetic.safe_add(...)` stability `stable`
- `proc safe_mul` signature `proc safe_mul(a: i64, b: i64) -> SafeResult {` example `arithmetic.safe_mul(...)` stability `stable`
- `proc and_i64` signature `proc and_i64(a: i64, b: i64) -> i64 {` example `arithmetic.and_i64(...)` stability `stable`
- `proc or_i64` signature `proc or_i64(a: i64, b: i64) -> i64 {` example `arithmetic.or_i64(...)` stability `stable`
- `proc xor_i64` signature `proc xor_i64(a: i64, b: i64) -> i64 {` example `arithmetic.xor_i64(...)` stability `stable`
- `proc shl` signature `proc shl(value: i64, bits: i64) -> i64 {` example `arithmetic.shl(...)` stability `stable`
- `proc shr` signature `proc shr(value: i64, bits: i64) -> i64 {` example `arithmetic.shr(...)` stability `stable`
- `proc sign_i64` signature `proc sign_i64(value: i64) -> i64 {` example `arithmetic.sign_i64(...)` stability `stable`
- `proc is_even` signature `proc is_even(value: i64) -> bool {` example `arithmetic.is_even(...)` stability `stable`
- `proc is_odd` signature `proc is_odd(value: i64) -> bool {` example `arithmetic.is_odd(...)` stability `stable`
- `proc popcount` signature `proc popcount(value: i64) -> i64 {` example `arithmetic.popcount(...)` stability `stable`
- `proc leading_zeros` signature `proc leading_zeros(value: i64) -> i64 {` example `arithmetic.leading_zeros(...)` stability `stable`
- `proc trailing_zeros` signature `proc trailing_zeros(value: i64) -> i64 {` example `arithmetic.trailing_zeros(...)` stability `stable`
- `proc rotl` signature `proc rotl(value: i64, bits: i64) -> i64 {` example `arithmetic.rotl(...)` stability `stable`
- `proc rotr` signature `proc rotr(value: i64, bits: i64) -> i64 {` example `arithmetic.rotr(...)` stability `stable`
- `proc saturating_add` signature `proc saturating_add(a: i64, b: i64) -> i64 {` example `arithmetic.saturating_add(...)` stability `stable`
- `proc saturating_sub` signature `proc saturating_sub(a: i64, b: i64) -> i64 {` example `arithmetic.saturating_sub(...)` stability `stable`
- `proc div_floor` signature `proc div_floor(a: i64, b: i64) -> i64 {` example `arithmetic.div_floor(...)` stability `stable`
- `proc div_ceil` signature `proc div_ceil(a: i64, b: i64) -> i64 {` example `arithmetic.div_ceil(...)` stability `stable`
- `proc floor_f64` signature `proc floor_f64(value: f64) -> i64 {` example `arithmetic.floor_f64(...)` stability `stable`
- `proc ceil_f64` signature `proc ceil_f64(value: f64) -> i64 {` example `arithmetic.ceil_f64(...)` stability `stable`
- `proc round_f64` signature `proc round_f64(value: f64) -> i64 {` example `arithmetic.round_f64(...)` stability `stable`
- `proc poly_eval` signature `proc poly_eval(coeffs: [f64], x: f64) -> f64 {` example `arithmetic.poly_eval(...)` stability `stable`
- `proc is_nan` signature `proc is_nan(value: f64) -> bool {` example `arithmetic.is_nan(...)` stability `stable`
- `proc is_inf` signature `proc is_inf(value: f64) -> bool {` example `arithmetic.is_inf(...)` stability `stable`
- `proc copysign` signature `proc copysign(magnitude: f64, sign_value: f64) -> f64 {` example `arithmetic.copysign(...)` stability `stable`
- `proc exp_f64` signature `proc exp_f64(value: f64) -> f64 {` example `arithmetic.exp_f64(...)` stability `stable`
- `proc log_f64` signature `proc log_f64(value: f64) -> f64 {` example `arithmetic.log_f64(...)` stability `stable`
- `proc sqrt_f64` signature `proc sqrt_f64(value: f64) -> f64 {` example `arithmetic.sqrt_f64(...)` stability `stable`
- `proc sin_f64` signature `proc sin_f64(value: f64) -> f64 {` example `arithmetic.sin_f64(...)` stability `stable`
- `proc cos_f64` signature `proc cos_f64(value: f64) -> f64 {` example `arithmetic.cos_f64(...)` stability `stable`
- `proc log10_f64` signature `proc log10_f64(value: f64) -> f64 {` example `arithmetic.log10_f64(...)` stability `stable`
- `proc fabs` signature `proc fabs(value: f64) -> f64 {` example `arithmetic.fabs(...)` stability `stable`
- `proc fmod` signature `proc fmod(a: f64, b: f64) -> f64 {` example `arithmetic.fmod(...)` stability `stable`
- `proc ceil` signature `proc ceil(value: f64) -> i64 {` example `arithmetic.ceil(...)` stability `stable`
- `proc floor` signature `proc floor(value: f64) -> i64 {` example `arithmetic.floor(...)` stability `stable`
- `proc rand_next` signature `proc rand_next(seed: i64) -> i64 {` example `arithmetic.rand_next(...)` stability `stable`
- `proc rand_f64` signature `proc rand_f64(seed: i64) -> f64 {` example `arithmetic.rand_f64(...)` stability `stable`
- `proc add_i128` signature `proc add_i128(a: I128, b: I128) -> I128 {` example `arithmetic.add_i128(...)` stability `stable`
- `proc mul_i128` signature `proc mul_i128(a: I128, b: I128) -> I128 {` example `arithmetic.mul_i128(...)` stability `stable`
- `proc __add_overflow` signature `proc __add_overflow(a: i64, b: i64) -> SafeResult {` example `arithmetic.__add_overflow(...)` stability `stable`
- `proc __mul_overflow` signature `proc __mul_overflow(a: i64, b: i64) -> SafeResult {` example `arithmetic.__mul_overflow(...)` stability `stable`
- `proc __popcnt` signature `proc __popcnt(value: i64) -> i64 {` example `arithmetic.__popcnt(...)` stability `stable`
- `proc __bsf` signature `proc __bsf(value: i64) -> i64 {` example `arithmetic.__bsf(...)` stability `stable`
- `proc __bsr` signature `proc __bsr(value: i64) -> i64 {` example `arithmetic.__bsr(...)` stability `stable`
- `proc arithmetic_version` signature `proc arithmetic_version() -> string {` example `arithmetic.arithmetic_version(...)` stability `stable`
- `proc arithmetic_ready` signature `proc arithmetic_ready() -> bool {` example `arithmetic.arithmetic_ready(...)` stability `stable`
- `proc arithmetic_selftest` signature `proc arithmetic_selftest() -> bool {` example `arithmetic.arithmetic_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/arrays.vitl`

Stability: `stable`

- `form RawSlice` signature `form RawSlice {` example `arrays.RawSlice` stability `stable`
- `form Vec` signature `form Vec {` example `arrays.Vec` stability `stable`
- `proc len` signature `proc len(values: [int]) -> int {` example `arrays.len(...)` stability `stable`
- `proc is_empty` signature `proc is_empty(values: [int]) -> bool {` example `arrays.is_empty(...)` stability `stable`
- `proc first_or` signature `proc first_or(values: [int], fallback: int) -> int {` example `arrays.first_or(...)` stability `stable`
- `proc last_or` signature `proc last_or(values: [int], fallback: int) -> int {` example `arrays.last_or(...)` stability `stable`
- `proc first` signature `proc first(values: [int]) -> int { give first_or(values, 0) }` example `arrays.first(...)` stability `stable`
- `proc last` signature `proc last(values: [int]) -> int { give last_or(values, 0) }` example `arrays.last(...)` stability `stable`
- `proc contains` signature `proc contains(values: [int], needle: int) -> bool {` example `arrays.contains(...)` stability `stable`
- `proc index_of` signature `proc index_of(values: [int], needle: int) -> int {` example `arrays.index_of(...)` stability `stable`
- `proc count` signature `proc count(values: [int], needle: int) -> int {` example `arrays.count(...)` stability `stable`
- `proc last_index_of` signature `proc last_index_of(values: [int], needle: int) -> int {` example `arrays.last_index_of(...)` stability `stable`
- `proc copy` signature `proc copy(values: [int]) -> [int] { give values }` example `arrays.copy(...)` stability `stable`
- `proc append` signature `proc append(values: [int], value: int) -> [int] { give values + [value] }` example `arrays.append(...)` stability `stable`
- `proc prepend` signature `proc prepend(values: [int], value: int) -> [int] { give [value] + values }` example `arrays.prepend(...)` stability `stable`
- `proc concat` signature `proc concat(a: [int], b: [int]) -> [int] { give a + b }` example `arrays.concat(...)` stability `stable`
- `proc push` signature `proc push(values: [int], value: int) -> [int] { give append(values, value) }` example `arrays.push(...)` stability `stable`
- `proc set_at` signature `proc set_at(values: [int], index: int, value: int) -> [int] {` example `arrays.set_at(...)` stability `stable`
- `proc swap` signature `proc swap(values: [int], i: int, j: int) -> [int] {` example `arrays.swap(...)` stability `stable`
- `proc clear` signature `proc clear(values: [int]) -> [int] { give [] }` example `arrays.clear(...)` stability `stable`
- `proc take` signature `proc take(values: [int], count0: int) -> [int] {` example `arrays.take(...)` stability `stable`
- `proc drop` signature `proc drop(values: [int], count0: int) -> [int] {` example `arrays.drop(...)` stability `stable`
- `proc pop` signature `proc pop(values: [int]) -> [int] {` example `arrays.pop(...)` stability `stable`
- `proc pop_at` signature `proc pop_at(values: [int], index: int) -> [int] { give remove_at(values, index) }` example `arrays.pop_at(...)` stability `stable`
- `proc insert` signature `proc insert(values: [int], index: int, value: int) -> [int] {` example `arrays.insert(...)` stability `stable`
- `proc remove_at` signature `proc remove_at(values: [int], index: int) -> [int] {` example `arrays.remove_at(...)` stability `stable`
- `proc repeat` signature `proc repeat(value: int, count0: int) -> [int] {` example `arrays.repeat(...)` stability `stable`
- `proc fill` signature `proc fill(values: [int], value: int) -> [int] { give repeat(value, values.len) }` example `arrays.fill(...)` stability `stable`
- `proc reverse_copy` signature `proc reverse_copy(values: [int]) -> [int] {` example `arrays.reverse_copy(...)` stability `stable`
- `proc reverse` signature `proc reverse(values: [int]) -> [int] { give reverse_copy(values) }` example `arrays.reverse(...)` stability `stable`
- `proc sum` signature `proc sum(values: [int]) -> int {` example `arrays.sum(...)` stability `stable`
- `proc min` signature `proc min(values: [int]) -> int {` example `arrays.min(...)` stability `stable`
- `proc max` signature `proc max(values: [int]) -> int {` example `arrays.max(...)` stability `stable`
- `proc mean_floor` signature `proc mean_floor(values: [int]) -> int {` example `arrays.mean_floor(...)` stability `stable`
- `proc mean_scaled` signature `proc mean_scaled(values: [int], scale: int) -> int {` example `arrays.mean_scaled(...)` stability `stable`
- `proc range` signature `proc range(values: [int]) -> int {` example `arrays.range(...)` stability `stable`
- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `arrays.abs_int(...)` stability `stable`
- `proc sum_abs` signature `proc sum_abs(values: [int]) -> int {` example `arrays.sum_abs(...)` stability `stable`
- `proc product` signature `proc product(values: [int]) -> int {` example `arrays.product(...)` stability `stable`
- `proc any_nonzero` signature `proc any_nonzero(values: [int]) -> bool {` example `arrays.any_nonzero(...)` stability `stable`
- `proc all_nonzero` signature `proc all_nonzero(values: [int]) -> bool {` example `arrays.all_nonzero(...)` stability `stable`
- `proc count_nonzero` signature `proc count_nonzero(values: [int]) -> int {` example `arrays.count_nonzero(...)` stability `stable`
- `proc prefix_min` signature `proc prefix_min(values: [int]) -> [int] {` example `arrays.prefix_min(...)` stability `stable`
- `proc prefix_max` signature `proc prefix_max(values: [int]) -> [int] {` example `arrays.prefix_max(...)` stability `stable`
- `proc clamp_each` signature `proc clamp_each(values: [int], low: int, high: int) -> [int] {` example `arrays.clamp_each(...)` stability `stable`
- `proc prefix_sum` signature `proc prefix_sum(values: [int]) -> [int] {` example `arrays.prefix_sum(...)` stability `stable`
- `proc tail` signature `proc tail(values: [int]) -> [int] { give drop(values, 1) }` example `arrays.tail(...)` stability `stable`
- `proc head` signature `proc head(values: [int]) -> int { give first(values) }` example `arrays.head(...)` stability `stable`
- `proc slice` signature `proc slice(values: [int], start: int, end: int) -> [int] {` example `arrays.slice(...)` stability `stable`
- `proc rotate_left` signature `proc rotate_left(values: [int], amount: int) -> [int] {` example `arrays.rotate_left(...)` stability `stable`
- `proc rotate_right` signature `proc rotate_right(values: [int], amount: int) -> [int] {` example `arrays.rotate_right(...)` stability `stable`
- `proc replace_slice` signature `proc replace_slice(values: [int], start: int, end: int, replacement: [int]) -> [int] {` example `arrays.replace_slice(...)` stability `stable`
- `proc splice` signature `proc splice(values: [int], start: int, delete_count: int, replacement: [int]) -> [int] {` example `arrays.splice(...)` stability `stable`
- `proc rotate` signature `proc rotate(values: [int], amount: int) -> [int] {` example `arrays.rotate(...)` stability `stable`
- `proc enumerate` signature `proc enumerate(values: [int]) -> [[int]] {` example `arrays.enumerate(...)` stability `stable`
- `proc zip` signature `proc zip(a: [int], b: [int]) -> [[int]] {` example `arrays.zip(...)` stability `stable`
- `proc chunks` signature `proc chunks(values: [int], size: int) -> [[int]] {` example `arrays.chunks(...)` stability `stable`
- `proc sort` signature `proc sort(values: [int]) -> [int] {` example `arrays.sort(...)` stability `stable`
- `proc binary_search` signature `proc binary_search(values: [int], needle: int) -> int {` example `arrays.binary_search(...)` stability `stable`
- `proc map_add` signature `proc map_add(values: [int], delta: int) -> [int] {` example `arrays.map_add(...)` stability `stable`
- `proc filter_positive` signature `proc filter_positive(values: [int]) -> [int] {` example `arrays.filter_positive(...)` stability `stable`
- `proc reduce_sum` signature `proc reduce_sum(values: [int]) -> int { give sum(values) }` example `arrays.reduce_sum(...)` stability `stable`
- `proc unique` signature `proc unique(values: [int]) -> [int] {` example `arrays.unique(...)` stability `stable`
- `proc intersect` signature `proc intersect(a: [int], b: [int]) -> [int] {` example `arrays.intersect(...)` stability `stable`
- `proc union_values` signature `proc union_values(a: [int], b: [int]) -> [int] {` example `arrays.union_values(...)` stability `stable`
- `proc difference` signature `proc difference(a: [int], b: [int]) -> [int] {` example `arrays.difference(...)` stability `stable`
- `proc memset` signature `proc memset(count0: int, value: int) -> [int] { give repeat(value, count0) }` example `arrays.memset(...)` stability `stable`
- `proc memcpy` signature `proc memcpy(values: [int]) -> [int] { give values }` example `arrays.memcpy(...)` stability `stable`
- `proc memcmp` signature `proc memcmp(a: [int], b: [int]) -> int {` example `arrays.memcmp(...)` stability `stable`
- `proc flatten` signature `proc flatten(values: [[int]]) -> [int] {` example `arrays.flatten(...)` stability `stable`
- `proc repeat_array` signature `proc repeat_array(values: [int], count0: int) -> [int] {` example `arrays.repeat_array(...)` stability `stable`
- `proc equals` signature `proc equals(a: [int], b: [int]) -> bool { give memcmp(a, b) == 0 }` example `arrays.equals(...)` stability `stable`
- `proc vec_new` signature `proc vec_new() -> Vec { give Vec { data: [] } }` example `arrays.vec_new(...)` stability `stable`
- `proc vec_with_capacity` signature `proc vec_with_capacity(capacity: int) -> Vec { give Vec { data: [] } }` example `arrays.vec_with_capacity(...)` stability `stable`
- `proc vec_push` signature `proc vec_push(v: Vec, value: int) -> Vec { give Vec { data: v.data + [value] } }` example `arrays.vec_push(...)` stability `stable`
- `proc vec_pop` signature `proc vec_pop(v: Vec) -> Vec { give Vec { data: pop(v.data) } }` example `arrays.vec_pop(...)` stability `stable`
- `proc vec_get` signature `proc vec_get(v: Vec, index: int) -> int { give first_or(slice(v.data, index, index + 1), 0) }` example `arrays.vec_get(...)` stability `stable`
- `proc vec_set` signature `proc vec_set(v: Vec, index: int, value: int) -> Vec { give Vec { data: set_at(v.data, index, value) } }` example `arrays.vec_set(...)` stability `stable`
- `proc vec_reserve` signature `proc vec_reserve(v: Vec, capacity: int) -> Vec { give v }` example `arrays.vec_reserve(...)` stability `stable`
- `proc vec_shrink` signature `proc vec_shrink(v: Vec) -> Vec { give v }` example `arrays.vec_shrink(...)` stability `stable`
- `proc sort_by` signature `proc sort_by(values: [int]) -> [int] { give sort(values) }` example `arrays.sort_by(...)` stability `stable`
- `proc qsort` signature `proc qsort(values: [int]) -> [int] { give sort(values) }` example `arrays.qsort(...)` stability `stable`
- `proc arrays_version` signature `proc arrays_version() -> string { give "max-1" }` example `arrays.arrays_version(...)` stability `stable`
- `proc arrays_ready` signature `proc arrays_ready() -> bool { give true }` example `arrays.arrays_ready(...)` stability `stable`
- `proc arrays_selftest` signature `proc arrays_selftest() -> bool {` example `arrays.arrays_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/calculus.vitl`

Stability: `stable`

- `const EPS` signature `const EPS: f64 = 0.000001` example `calculus.EPS` stability `stable`
- `const MAX_ITER` signature `const MAX_ITER: i64 = 32` example `calculus.MAX_ITER` stability `stable`
- `form Complex` signature `form Complex { re: f64, im: f64 }` example `calculus.Complex` stability `stable`
- `form CSR` signature `form CSR { rows: [int], cols: [int], values: [f64], width: int }` example `calculus.CSR` stability `stable`
- `form Dual` signature `form Dual { real: f64, dual: f64 }` example `calculus.Dual` stability `stable`
- `proc poly_eval` signature `proc poly_eval(coeffs: [f64], x: f64) -> f64 {` example `calculus.poly_eval(...)` stability `stable`
- `proc deriv_forward` signature `proc deriv_forward(coeffs: [f64], x: f64, h: f64) -> f64 {` example `calculus.deriv_forward(...)` stability `stable`
- `proc deriv_backward` signature `proc deriv_backward(coeffs: [f64], x: f64, h: f64) -> f64 {` example `calculus.deriv_backward(...)` stability `stable`
- `proc deriv_central` signature `proc deriv_central(coeffs: [f64], x: f64, h: f64) -> f64 {` example `calculus.deriv_central(...)` stability `stable`
- `proc deriv2` signature `proc deriv2(coeffs: [f64], x: f64, h: f64) -> f64 {` example `calculus.deriv2(...)` stability `stable`
- `proc gradient` signature `proc gradient(coeff_x: [f64], coeff_y: [f64], x: f64, y: f64, h: f64) -> [f64] {` example `calculus.gradient(...)` stability `stable`
- `proc integrate_trap` signature `proc integrate_trap(samples: [f64], h: f64) -> f64 {` example `calculus.integrate_trap(...)` stability `stable`
- `proc integrate_simpson` signature `proc integrate_simpson(samples: [f64], h: f64) -> f64 {` example `calculus.integrate_simpson(...)` stability `stable`
- `proc limit` signature `proc limit(left: f64, right: f64) -> f64 {` example `calculus.limit(...)` stability `stable`
- `proc deriv_coeffs` signature `proc deriv_coeffs(coeffs: [f64]) -> [f64] {` example `calculus.deriv_coeffs(...)` stability `stable`
- `proc newton` signature `proc newton(coeffs: [f64], guess: f64) -> f64 {` example `calculus.newton(...)` stability `stable`
- `proc bisection` signature `proc bisection(coeffs: [f64], low0: f64, high0: f64) -> f64 {` example `calculus.bisection(...)` stability `stable`
- `proc taylor_exp` signature `proc taylor_exp(x: f64, terms: int) -> f64 {` example `calculus.taylor_exp(...)` stability `stable`
- `proc taylor_sin` signature `proc taylor_sin(x: f64, terms: int) -> f64 {` example `calculus.taylor_sin(...)` stability `stable`
- `proc euler` signature `proc euler(y0: f64, step: f64, slope: [f64]) -> [f64] {` example `calculus.euler(...)` stability `stable`
- `proc rk4` signature `proc rk4(y0: f64, step: f64, slopes: [[f64]]) -> [f64] {` example `calculus.rk4(...)` stability `stable`
- `proc gradient_descent` signature `proc gradient_descent(start: f64, grad: [f64], rate: f64) -> f64 {` example `calculus.gradient_descent(...)` stability `stable`
- `proc newton_opt` signature `proc newton_opt(start: f64, first: [f64], second: [f64]) -> f64 {` example `calculus.newton_opt(...)` stability `stable`
- `proc integrate_montecarlo` signature `proc integrate_montecarlo(samples: [f64], low: f64, high: f64) -> f64 {` example `calculus.integrate_montecarlo(...)` stability `stable`
- `proc adaptive_simpson` signature `proc adaptive_simpson(samples: [f64], h: f64) -> f64 {` example `calculus.adaptive_simpson(...)` stability `stable`
- `proc recurse` signature `proc recurse(value: int) -> int {` example `calculus.recurse(...)` stability `stable`
- `proc gradient_nd` signature `proc gradient_nd(surface: [[f64]]) -> [f64] {` example `calculus.gradient_nd(...)` stability `stable`
- `proc stable_sum` signature `proc stable_sum(values: [f64]) -> f64 {` example `calculus.stable_sum(...)` stability `stable`
- `proc stable_mean` signature `proc stable_mean(values: [f64]) -> f64 {` example `calculus.stable_mean(...)` stability `stable`
- `proc poly_root_quadratic` signature `proc poly_root_quadratic(a: f64, b: f64, c: f64) -> [f64] {` example `calculus.poly_root_quadratic(...)` stability `stable`
- `proc dot` signature `proc dot(a: [f64], b: [f64]) -> f64 {` example `calculus.dot(...)` stability `stable`
- `proc mat_mul` signature `proc mat_mul(a: [[f64]], b: [[f64]]) -> [[f64]] {` example `calculus.mat_mul(...)` stability `stable`
- `proc mat_vec` signature `proc mat_vec(m: [[f64]], v: [f64]) -> [f64] {` example `calculus.mat_vec(...)` stability `stable`
- `proc solve_gauss` signature `proc solve_gauss(a: [[f64]], b: [f64]) -> [f64] {` example `calculus.solve_gauss(...)` stability `stable`
- `proc c_add` signature `proc c_add(a: Complex, b: Complex) -> Complex {` example `calculus.c_add(...)` stability `stable`
- `proc c_sub` signature `proc c_sub(a: Complex, b: Complex) -> Complex {` example `calculus.c_sub(...)` stability `stable`
- `proc c_mul` signature `proc c_mul(a: Complex, b: Complex) -> Complex {` example `calculus.c_mul(...)` stability `stable`
- `proc fft` signature `proc fft(values: [Complex]) -> [Complex] {` example `calculus.fft(...)` stability `stable`
- `proc variance` signature `proc variance(values: [f64]) -> f64 {` example `calculus.variance(...)` stability `stable`
- `proc stddev` signature `proc stddev(values: [f64]) -> f64 {` example `calculus.stddev(...)` stability `stable`
- `proc normal_pdf` signature `proc normal_pdf(x: f64, mean0: f64, std0: f64) -> f64 {` example `calculus.normal_pdf(...)` stability `stable`
- `proc rk45` signature `proc rk45(y0: f64, step: f64, slopes: [[f64]]) -> [f64] {` example `calculus.rk45(...)` stability `stable`
- `proc eigen_power` signature `proc eigen_power(matrix: [[f64]], vector: [f64], iters: int) -> [f64] {` example `calculus.eigen_power(...)` stability `stable`
- `proc csr_matvec` signature `proc csr_matvec(m: CSR, v: [f64]) -> [f64] {` example `calculus.csr_matvec(...)` stability `stable`
- `proc d_add` signature `proc d_add(a: Dual, b: Dual) -> Dual {` example `calculus.d_add(...)` stability `stable`
- `proc d_mul` signature `proc d_mul(a: Dual, b: Dual) -> Dual {` example `calculus.d_mul(...)` stability `stable`
- `proc d_sin` signature `proc d_sin(a: Dual) -> Dual {` example `calculus.d_sin(...)` stability `stable`
- `proc autodiff` signature `proc autodiff(a: Dual) -> Dual {` example `calculus.autodiff(...)` stability `stable`
- `proc gamma` signature `proc gamma(value: f64) -> f64 {` example `calculus.gamma(...)` stability `stable`
- `proc erf` signature `proc erf(value: f64) -> f64 {` example `calculus.erf(...)` stability `stable`
- `proc parallel_map` signature `proc parallel_map(values: [f64]) -> [f64] {` example `calculus.parallel_map(...)` stability `stable`
- `proc parallel_reduce` signature `proc parallel_reduce(values: [f64]) -> f64 {` example `calculus.parallel_reduce(...)` stability `stable`
- `proc mean_f64` signature `proc mean_f64(values: [f64]) -> f64 {` example `calculus.mean_f64(...)` stability `stable`
- `proc abs_local` signature `proc abs_local(value: f64) -> f64 {` example `calculus.abs_local(...)` stability `stable`
- `proc sqrt_local` signature `proc sqrt_local(value: f64) -> f64 {` example `calculus.sqrt_local(...)` stability `stable`
- `proc exp_local` signature `proc exp_local(value: f64) -> f64 {` example `calculus.exp_local(...)` stability `stable`
- `proc pow_simple` signature `proc pow_simple(base: f64, exp: int) -> f64 {` example `calculus.pow_simple(...)` stability `stable`
- `proc factorial_f64` signature `proc factorial_f64(value: int) -> f64 {` example `calculus.factorial_f64(...)` stability `stable`
- `proc calculus_version` signature `proc calculus_version() -> string {` example `calculus.calculus_version(...)` stability `stable`
- `proc calculus_ready` signature `proc calculus_ready() -> bool {` example `calculus.calculus_ready(...)` stability `stable`
- `proc calculus_selftest` signature `proc calculus_selftest() -> bool {` example `calculus.calculus_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/comparison.vitl`

Stability: `stable`

- `const EPS` signature `const EPS: f64 = 0.000001` example `comparison.EPS` stability `stable`
- `proc eq` signature `proc eq(a: int, b: int) -> bool { give a == b }` example `comparison.eq(...)` stability `stable`
- `proc lt` signature `proc lt(a: int, b: int) -> bool { give a < b }` example `comparison.lt(...)` stability `stable`
- `proc gt` signature `proc gt(a: int, b: int) -> bool { give a > b }` example `comparison.gt(...)` stability `stable`
- `proc is_nan` signature `proc is_nan(value: f64) -> bool { give false }` example `comparison.is_nan(...)` stability `stable`
- `proc is_inf` signature `proc is_inf(value: f64) -> bool { give value > 1000000000000000000.0 or value < 0.0 - 1000000000000000000.0 }` example `comparison.is_inf(...)` stability `stable`
- `proc min` signature `proc min(a: int, b: int) -> int { if a < b { give a } give b }` example `comparison.min(...)` stability `stable`
- `proc max` signature `proc max(a: int, b: int) -> int { if a > b { give a } give b }` example `comparison.max(...)` stability `stable`
- `proc clamp` signature `proc clamp(value: int, low: int, high: int) -> int { if value < low { give low } if value > high { give high } give value }` example `comparison.clamp(...)` stability `stable`
- `proc abs` signature `proc abs(value: int) -> int { if value < 0 { give 0 - value } give value }` example `comparison.abs(...)` stability `stable`
- `proc sign` signature `proc sign(value: int) -> int { if value > 0 { give 1 } if value < 0 { give -1 } give 0 }` example `comparison.sign(...)` stability `stable`
- `proc between` signature `proc between(value: int, low: int, high: int) -> bool { give value > low and value < high }` example `comparison.between(...)` stability `stable`
- `proc in_range` signature `proc in_range(value: int, low: int, high: int) -> bool { give value >= low and value <= high }` example `comparison.in_range(...)` stability `stable`
- `proc compare` signature `proc compare(a: int, b: int) -> int { if a < b { give -1 } if a > b { give 1 } give 0 }` example `comparison.compare(...)` stability `stable`
- `proc cmp_reverse` signature `proc cmp_reverse(a: int, b: int) -> int { give 0 - compare(a, b) }` example `comparison.cmp_reverse(...)` stability `stable`
- `proc min_f64` signature `proc min_f64(a: f64, b: f64) -> f64 { if a < b { give a } give b }` example `comparison.min_f64(...)` stability `stable`
- `proc max_f64` signature `proc max_f64(a: f64, b: f64) -> f64 { if a > b { give a } give b }` example `comparison.max_f64(...)` stability `stable`
- `proc clamp_f64` signature `proc clamp_f64(value: f64, low: f64, high: f64) -> f64 { if value < low { give low } if value > high { give high } give value }` example `comparison.clamp_f64(...)` stability `stable`
- `proc abs_f64` signature `proc abs_f64(value: f64) -> f64 { if value < 0.0 { give 0.0 - value } give value }` example `comparison.abs_f64(...)` stability `stable`
- `proc sign_f64` signature `proc sign_f64(value: f64) -> f64 { if value > 0.0 { give 1.0 } if value < 0.0 { give -1.0 } give 0.0 }` example `comparison.sign_f64(...)` stability `stable`
- `proc compare_f64` signature `proc compare_f64(a: f64, b: f64) -> int { if a < b { give -1 } if a > b { give 1 } give 0 }` example `comparison.compare_f64(...)` stability `stable`
- `proc approx_eq` signature `proc approx_eq(a: f64, b: f64) -> bool {` example `comparison.approx_eq(...)` stability `stable`
- `proc approx_lt` signature `proc approx_lt(a: f64, b: f64) -> bool { give a < b and not approx_eq(a, b) }` example `comparison.approx_lt(...)` stability `stable`
- `proc approx_gt` signature `proc approx_gt(a: f64, b: f64) -> bool { give a > b and not approx_eq(a, b) }` example `comparison.approx_gt(...)` stability `stable`
- `proc approx_between` signature `proc approx_between(value: f64, low: f64, high: f64) -> bool { give value > low - EPS and value < high + EPS }` example `comparison.approx_between(...)` stability `stable`
- `proc approx_in_range` signature `proc approx_in_range(value: f64, low: f64, high: f64) -> bool { give value >= low - EPS and value <= high + EPS }` example `comparison.approx_in_range(...)` stability `stable`
- `proc is_sorted` signature `proc is_sorted(values: [int]) -> bool { let i: int = 1 while i < values.len { if values[i] < values[i - 1] { give false } set i = i + 1 } give true }` example `comparison.is_sorted(...)` stability `stable`
- `proc is_sorted_desc` signature `proc is_sorted_desc(values: [int]) -> bool { let i: int = 1 while i < values.len { if values[i] > values[i - 1] { give false } set i = i + 1 } give true }` example `comparison.is_sorted_desc(...)` stability `stable`
- `proc is_sorted_f64` signature `proc is_sorted_f64(values: [f64]) -> bool { let i: int = 1 while i < values.len { if values[i] < values[i - 1] { give false } set i = i + 1 } give true }` example `comparison.is_sorted_f64(...)` stability `stable`
- `proc is_sorted_desc_f64` signature `proc is_sorted_desc_f64(values: [f64]) -> bool { let i: int = 1 while i < values.len { if values[i] > values[i - 1] { give false } set i = i + 1 } give true }` example `comparison.is_sorted_desc_f64(...)` stability `stable`
- `proc argmin` signature `proc argmin(values: [int]) -> int { if values.len == 0 { give -1 } let best: int = 0 let i: int = 1 while i < values.len { if values[i] < values[best] { set best = i } set i = i + 1 } give best }` example `comparison.argmin(...)` stability `stable`
- `proc argmax` signature `proc argmax(values: [int]) -> int { if values.len == 0 { give -1 } let best: int = 0 let i: int = 1 while i < values.len { if values[i] > values[best] { set best = i } set i = i + 1 } give best }` example `comparison.argmax(...)` stability `stable`
- `proc argmin_f64` signature `proc argmin_f64(values: [f64]) -> int { if values.len == 0 { give -1 } let best: int = 0 let i: int = 1 while i < values.len { if values[i] < values[best] { set best = i } set i = i + 1 } give best }` example `comparison.argmin_f64(...)` stability `stable`
- `proc argmax_f64` signature `proc argmax_f64(values: [f64]) -> int { if values.len == 0 { give -1 } let best: int = 0 let i: int = 1 while i < values.len { if values[i] > values[best] { set best = i } set i = i + 1 } give best }` example `comparison.argmax_f64(...)` stability `stable`
- `proc all` signature `proc all(values: [bool]) -> bool { let i: int = 0 while i < values.len { if not values[i] { give false } set i = i + 1 } give values.len > 0 }` example `comparison.all(...)` stability `stable`
- `proc any` signature `proc any(values: [bool]) -> bool { let i: int = 0 while i < values.len { if values[i] { give true } set i = i + 1 } give false }` example `comparison.any(...)` stability `stable`
- `proc none` signature `proc none(values: [bool]) -> bool { give not any(values) }` example `comparison.none(...)` stability `stable`
- `proc count_true` signature `proc count_true(values: [bool]) -> int { let total: int = 0 let i: int = 0 while i < values.len { if values[i] { set total = total + 1 } set i = i + 1 } give total }` example `comparison.count_true(...)` stability `stable`
- `proc min_fast` signature `proc min_fast(a: int, b: int) -> int { give min(a, b) }` example `comparison.min_fast(...)` stability `stable`
- `proc max_fast` signature `proc max_fast(a: int, b: int) -> int { give max(a, b) }` example `comparison.max_fast(...)` stability `stable`
- `proc compare_generic` signature `proc compare_generic(a: int, b: int) -> int { give compare(a, b) }` example `comparison.compare_generic(...)` stability `stable`
- `proc min_generic` signature `proc min_generic(a: int, b: int) -> int { give min(a, b) }` example `comparison.min_generic(...)` stability `stable`
- `proc max_generic` signature `proc max_generic(a: int, b: int) -> int { give max(a, b) }` example `comparison.max_generic(...)` stability `stable`
- `proc clamp_generic` signature `proc clamp_generic(value: int, low: int, high: int) -> int { give clamp(value, low, high) }` example `comparison.clamp_generic(...)` stability `stable`
- `proc is_sorted_generic` signature `proc is_sorted_generic(values: [int]) -> bool { give is_sorted(values) }` example `comparison.is_sorted_generic(...)` stability `stable`
- `proc comparison_version` signature `proc comparison_version() -> string { give "max-1" }` example `comparison.comparison_version(...)` stability `stable`
- `proc comparison_ready` signature `proc comparison_ready() -> bool { give true }` example `comparison.comparison_ready(...)` stability `stable`
- `proc comparison_selftest` signature `proc comparison_selftest() -> bool {` example `comparison.comparison_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/complex.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `complex.abs_int(...)` stability `stable`
- `proc sqrt_floor` signature `proc sqrt_floor(value: int) -> int {` example `complex.sqrt_floor(...)` stability `stable`
- `proc complex_pair` signature `proc complex_pair(real: int, imag: int) -> [int] {` example `complex.complex_pair(...)` stability `stable`
- `proc complex_zero` signature `proc complex_zero() -> [int] {` example `complex.complex_zero(...)` stability `stable`
- `proc complex_one` signature `proc complex_one() -> [int] {` example `complex.complex_one(...)` stability `stable`
- `proc complex_i` signature `proc complex_i() -> [int] {` example `complex.complex_i(...)` stability `stable`
- `proc complex_is_valid` signature `proc complex_is_valid(value: [int]) -> bool {` example `complex.complex_is_valid(...)` stability `stable`
- `proc complex_real` signature `proc complex_real(value: [int]) -> int {` example `complex.complex_real(...)` stability `stable`
- `proc complex_imag` signature `proc complex_imag(value: [int]) -> int {` example `complex.complex_imag(...)` stability `stable`
- `proc complex_clone` signature `proc complex_clone(value: [int]) -> [int] {` example `complex.complex_clone(...)` stability `stable`
- `proc complex_equal` signature `proc complex_equal(a: [int], b: [int]) -> bool {` example `complex.complex_equal(...)` stability `stable`
- `proc complex_is_zero` signature `proc complex_is_zero(value: [int]) -> bool {` example `complex.complex_is_zero(...)` stability `stable`
- `proc complex_is_real` signature `proc complex_is_real(value: [int]) -> bool {` example `complex.complex_is_real(...)` stability `stable`
- `proc complex_is_imaginary` signature `proc complex_is_imaginary(value: [int]) -> bool {` example `complex.complex_is_imaginary(...)` stability `stable`
- `proc complex_is_unit` signature `proc complex_is_unit(value: [int]) -> bool {` example `complex.complex_is_unit(...)` stability `stable`
- `proc complex_add` signature `proc complex_add(a: [int], b: [int]) -> [int] {` example `complex.complex_add(...)` stability `stable`
- `proc complex_sub` signature `proc complex_sub(a: [int], b: [int]) -> [int] {` example `complex.complex_sub(...)` stability `stable`
- `proc complex_neg` signature `proc complex_neg(value: [int]) -> [int] {` example `complex.complex_neg(...)` stability `stable`
- `proc complex_scale` signature `proc complex_scale(value: [int], scalar: int) -> [int] {` example `complex.complex_scale(...)` stability `stable`
- `proc complex_mul` signature `proc complex_mul(a: [int], b: [int]) -> [int] {` example `complex.complex_mul(...)` stability `stable`
- `proc complex_square` signature `proc complex_square(value: [int]) -> [int] {` example `complex.complex_square(...)` stability `stable`
- `proc complex_cube` signature `proc complex_cube(value: [int]) -> [int] {` example `complex.complex_cube(...)` stability `stable`
- `proc complex_conj` signature `proc complex_conj(value: [int]) -> [int] {` example `complex.complex_conj(...)` stability `stable`
- `proc complex_abs_sq` signature `proc complex_abs_sq(value: [int]) -> int {` example `complex.complex_abs_sq(...)` stability `stable`
- `proc complex_norm` signature `proc complex_norm(value: [int]) -> int {` example `complex.complex_norm(...)` stability `stable`
- `proc complex_abs` signature `proc complex_abs(value: [int]) -> int {` example `complex.complex_abs(...)` stability `stable`
- `proc complex_manhattan` signature `proc complex_manhattan(value: [int]) -> int {` example `complex.complex_manhattan(...)` stability `stable`
- `proc complex_chebyshev` signature `proc complex_chebyshev(value: [int]) -> int {` example `complex.complex_chebyshev(...)` stability `stable`
- `proc complex_dot` signature `proc complex_dot(a: [int], b: [int]) -> int {` example `complex.complex_dot(...)` stability `stable`
- `proc complex_cross` signature `proc complex_cross(a: [int], b: [int]) -> int {` example `complex.complex_cross(...)` stability `stable`
- `proc complex_distance_sq` signature `proc complex_distance_sq(a: [int], b: [int]) -> int {` example `complex.complex_distance_sq(...)` stability `stable`
- `proc complex_distance` signature `proc complex_distance(a: [int], b: [int]) -> int {` example `complex.complex_distance(...)` stability `stable`
- `proc complex_arg_quadrant` signature `proc complex_arg_quadrant(value: [int]) -> int {` example `complex.complex_arg_quadrant(...)` stability `stable`
- `proc complex_inv` signature `proc complex_inv(value: [int]) -> [int] {` example `complex.complex_inv(...)` stability `stable`
- `proc complex_div` signature `proc complex_div(a: [int], b: [int]) -> [int] {` example `complex.complex_div(...)` stability `stable`
- `proc complex_has_inverse` signature `proc complex_has_inverse(value: [int]) -> bool {` example `complex.complex_has_inverse(...)` stability `stable`
- `proc complex_pow` signature `proc complex_pow(value: [int], exponent: int) -> [int] {` example `complex.complex_pow(...)` stability `stable`
- `proc complex_from_real` signature `proc complex_from_real(real: int) -> [int] {` example `complex.complex_from_real(...)` stability `stable`
- `proc complex_swap` signature `proc complex_swap(value: [int]) -> [int] {` example `complex.complex_swap(...)` stability `stable`
- `proc complex_perp_left` signature `proc complex_perp_left(value: [int]) -> [int] {` example `complex.complex_perp_left(...)` stability `stable`
- `proc complex_perp_right` signature `proc complex_perp_right(value: [int]) -> [int] {` example `complex.complex_perp_right(...)` stability `stable`
- `proc complex_mul_i` signature `proc complex_mul_i(value: [int]) -> [int] {` example `complex.complex_mul_i(...)` stability `stable`
- `proc complex_mul_neg_i` signature `proc complex_mul_neg_i(value: [int]) -> [int] {` example `complex.complex_mul_neg_i(...)` stability `stable`
- `proc complex_powers` signature `proc complex_powers(value: [int], count: int) -> [[int]] {` example `complex.complex_powers(...)` stability `stable`
- `proc complex_re` signature `proc complex_re(value: [int]) -> int {` example `complex.complex_re(...)` stability `stable`
- `proc complex_im` signature `proc complex_im(value: [int]) -> int {` example `complex.complex_im(...)` stability `stable`
- `proc complex_version` signature `proc complex_version() -> string {` example `complex.complex_version(...)` stability `stable`
- `proc complex_ready` signature `proc complex_ready() -> bool {` example `complex.complex_ready(...)` stability `stable`
- `proc complex_selftest` signature `proc complex_selftest() -> bool {` example `complex.complex_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/geometry.vitl`

Stability: `stable`

- `const PI_TIMES_1000000` signature `const PI_TIMES_1000000: int = 3141592` example `geometry.PI_TIMES_1000000` stability `stable`
- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `geometry.abs_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `geometry.min_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `geometry.max_int(...)` stability `stable`
- `proc clamp_int` signature `proc clamp_int(value: int, low: int, high: int) -> int {` example `geometry.clamp_int(...)` stability `stable`
- `proc sign_int` signature `proc sign_int(value: int) -> int {` example `geometry.sign_int(...)` stability `stable`
- `proc sqrt_floor` signature `proc sqrt_floor(value: int) -> int {` example `geometry.sqrt_floor(...)` stability `stable`
- `proc point2` signature `proc point2(x: int, y: int) -> [int] {` example `geometry.point2(...)` stability `stable`
- `proc vec2` signature `proc vec2(x: int, y: int) -> [int] {` example `geometry.vec2(...)` stability `stable`
- `proc point2_zero` signature `proc point2_zero() -> [int] {` example `geometry.point2_zero(...)` stability `stable`
- `proc point2_is_valid` signature `proc point2_is_valid(p: [int]) -> bool {` example `geometry.point2_is_valid(...)` stability `stable`
- `proc point2_x` signature `proc point2_x(p: [int]) -> int {` example `geometry.point2_x(...)` stability `stable`
- `proc point2_y` signature `proc point2_y(p: [int]) -> int {` example `geometry.point2_y(...)` stability `stable`
- `proc vec2_add` signature `proc vec2_add(a: [int], b: [int]) -> [int] {` example `geometry.vec2_add(...)` stability `stable`
- `proc vec2_sub` signature `proc vec2_sub(a: [int], b: [int]) -> [int] {` example `geometry.vec2_sub(...)` stability `stable`
- `proc vec2_neg` signature `proc vec2_neg(v: [int]) -> [int] {` example `geometry.vec2_neg(...)` stability `stable`
- `proc vec2_scale` signature `proc vec2_scale(v: [int], k: int) -> [int] {` example `geometry.vec2_scale(...)` stability `stable`
- `proc vec2_equal` signature `proc vec2_equal(a: [int], b: [int]) -> bool {` example `geometry.vec2_equal(...)` stability `stable`
- `proc vec2_dot` signature `proc vec2_dot(a: [int], b: [int]) -> int {` example `geometry.vec2_dot(...)` stability `stable`
- `proc vec2_cross` signature `proc vec2_cross(a: [int], b: [int]) -> int {` example `geometry.vec2_cross(...)` stability `stable`
- `proc vec2_length_sq` signature `proc vec2_length_sq(v: [int]) -> int {` example `geometry.vec2_length_sq(...)` stability `stable`
- `proc vec2_manhattan` signature `proc vec2_manhattan(v: [int]) -> int {` example `geometry.vec2_manhattan(...)` stability `stable`
- `proc vec2_chebyshev` signature `proc vec2_chebyshev(v: [int]) -> int {` example `geometry.vec2_chebyshev(...)` stability `stable`
- `proc vec2_perp_left` signature `proc vec2_perp_left(v: [int]) -> [int] {` example `geometry.vec2_perp_left(...)` stability `stable`
- `proc vec2_perp_right` signature `proc vec2_perp_right(v: [int]) -> [int] {` example `geometry.vec2_perp_right(...)` stability `stable`
- `proc midpoint` signature `proc midpoint(ax: int, ay: int, bx: int, by: int) -> [int] {` example `geometry.midpoint(...)` stability `stable`
- `proc midpoint_points` signature `proc midpoint_points(a: [int], b: [int]) -> [int] {` example `geometry.midpoint_points(...)` stability `stable`
- `proc rect_area` signature `proc rect_area(width: int, height: int) -> int {` example `geometry.rect_area(...)` stability `stable`
- `proc rect_perimeter` signature `proc rect_perimeter(width: int, height: int) -> int {` example `geometry.rect_perimeter(...)` stability `stable`
- `proc rect_is_valid` signature `proc rect_is_valid(width: int, height: int) -> bool {` example `geometry.rect_is_valid(...)` stability `stable`
- `proc rect_contains` signature `proc rect_contains(px: int, py: int, x: int, y: int, width: int, height: int) -> bool {` example `geometry.rect_contains(...)` stability `stable`
- `proc rect_intersects` signature `proc rect_intersects(ax: int, ay: int, aw: int, ah: int, bx: int, by: int, bw: int, bh: int) -> bool {` example `geometry.rect_intersects(...)` stability `stable`
- `proc rect_intersection_area` signature `proc rect_intersection_area(ax: int, ay: int, aw: int, ah: int, bx: int, by: int, bw: int, bh: int) -> int {` example `geometry.rect_intersection_area(...)` stability `stable`
- `proc rect_union_area` signature `proc rect_union_area(ax: int, ay: int, aw: int, ah: int, bx: int, by: int, bw: int, bh: int) -> int {` example `geometry.rect_union_area(...)` stability `stable`
- `proc triangle_area2` signature `proc triangle_area2(base: int, height: int) -> int {` example `geometry.triangle_area2(...)` stability `stable`
- `proc triangle_area` signature `proc triangle_area(base: int, height: int) -> int {` example `geometry.triangle_area(...)` stability `stable`
- `proc triangle_area2_points` signature `proc triangle_area2_points(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> int {` example `geometry.triangle_area2_points(...)` stability `stable`
- `proc triangle_area_points` signature `proc triangle_area_points(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> int {` example `geometry.triangle_area_points(...)` stability `stable`
- `proc triangle_perimeter_manhattan` signature `proc triangle_perimeter_manhattan(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> int {` example `geometry.triangle_perimeter_manhattan(...)` stability `stable`
- `proc distance_sq` signature `proc distance_sq(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.distance_sq(...)` stability `stable`
- `proc distance` signature `proc distance(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.distance(...)` stability `stable`
- `proc manhattan_distance` signature `proc manhattan_distance(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.manhattan_distance(...)` stability `stable`
- `proc chebyshev_distance` signature `proc chebyshev_distance(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.chebyshev_distance(...)` stability `stable`
- `proc norm1` signature `proc norm1(x: int, y: int) -> int {` example `geometry.norm1(...)` stability `stable`
- `proc norm2_sq` signature `proc norm2_sq(x: int, y: int) -> int {` example `geometry.norm2_sq(...)` stability `stable`
- `proc norm2` signature `proc norm2(x: int, y: int) -> int {` example `geometry.norm2(...)` stability `stable`
- `proc norm_inf` signature `proc norm_inf(x: int, y: int) -> int {` example `geometry.norm_inf(...)` stability `stable`
- `proc point_to_origin_distance_sq` signature `proc point_to_origin_distance_sq(x: int, y: int) -> int {` example `geometry.point_to_origin_distance_sq(...)` stability `stable`
- `proc point_to_origin_distance` signature `proc point_to_origin_distance(x: int, y: int) -> int {` example `geometry.point_to_origin_distance(...)` stability `stable`
- `proc cross2` signature `proc cross2(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.cross2(...)` stability `stable`
- `proc dot2` signature `proc dot2(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.dot2(...)` stability `stable`
- `proc orientation` signature `proc orientation(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> int {` example `geometry.orientation(...)` stability `stable`
- `proc collinear` signature `proc collinear(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> bool {` example `geometry.collinear(...)` stability `stable`
- `proc left_turn` signature `proc left_turn(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> bool {` example `geometry.left_turn(...)` stability `stable`
- `proc right_turn` signature `proc right_turn(ax: int, ay: int, bx: int, by: int, cx: int, cy: int) -> bool {` example `geometry.right_turn(...)` stability `stable`
- `proc on_segment` signature `proc on_segment(ax: int, ay: int, bx: int, by: int, px: int, py: int) -> bool {` example `geometry.on_segment(...)` stability `stable`
- `proc segments_intersect` signature `proc segments_intersect(ax: int, ay: int, bx: int, by: int, cx: int, cy: int, dx: int, dy: int) -> bool {` example `geometry.segments_intersect(...)` stability `stable`
- `proc segment_length_sq` signature `proc segment_length_sq(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.segment_length_sq(...)` stability `stable`
- `proc segment_length` signature `proc segment_length(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.segment_length(...)` stability `stable`
- `proc circle_area_times_1000000` signature `proc circle_area_times_1000000(radius: int) -> int {` example `geometry.circle_area_times_1000000(...)` stability `stable`
- `proc circle_perimeter_times_1000000` signature `proc circle_perimeter_times_1000000(radius: int) -> int {` example `geometry.circle_perimeter_times_1000000(...)` stability `stable`
- `proc point_in_circle` signature `proc point_in_circle(px: int, py: int, cx: int, cy: int, radius: int) -> bool {` example `geometry.point_in_circle(...)` stability `stable`
- `proc bbox2` signature `proc bbox2(ax: int, ay: int, bx: int, by: int) -> [int] {` example `geometry.bbox2(...)` stability `stable`
- `proc bbox_contains` signature `proc bbox_contains(box: [int], px: int, py: int) -> bool {` example `geometry.bbox_contains(...)` stability `stable`
- `proc bbox_width` signature `proc bbox_width(box: [int]) -> int {` example `geometry.bbox_width(...)` stability `stable`
- `proc bbox_height` signature `proc bbox_height(box: [int]) -> int {` example `geometry.bbox_height(...)` stability `stable`
- `proc bbox_area` signature `proc bbox_area(box: [int]) -> int {` example `geometry.bbox_area(...)` stability `stable`
- `proc l1_distance` signature `proc l1_distance(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.l1_distance(...)` stability `stable`
- `proc l2_distance_sq` signature `proc l2_distance_sq(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.l2_distance_sq(...)` stability `stable`
- `proc linf_distance` signature `proc linf_distance(ax: int, ay: int, bx: int, by: int) -> int {` example `geometry.linf_distance(...)` stability `stable`
- `proc geometry_version` signature `proc geometry_version() -> string {` example `geometry.geometry_version(...)` stability `stable`
- `proc geometry_ready` signature `proc geometry_ready() -> bool {` example `geometry.geometry_ready(...)` stability `stable`
- `proc geometry_selftest` signature `proc geometry_selftest() -> bool {` example `geometry.geometry_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/matrix.vitl`

Stability: `stable`

- `proc mat2` signature `proc mat2(a: int, b: int, c: int, d: int) -> [int] { give [a, b, c, d] }` example `matrix.mat2(...)` stability `stable`
- `proc mat2_zero` signature `proc mat2_zero() -> [int] { give [0, 0, 0, 0] }` example `matrix.mat2_zero(...)` stability `stable`
- `proc mat2_identity` signature `proc mat2_identity() -> [int] { give [1, 0, 0, 1] }` example `matrix.mat2_identity(...)` stability `stable`
- `proc mat2_diag` signature `proc mat2_diag(a: int, d: int) -> [int] { give [a, 0, 0, d] }` example `matrix.mat2_diag(...)` stability `stable`
- `proc mat2_is_valid` signature `proc mat2_is_valid(m: [int]) -> bool { give m.len == 4 }` example `matrix.mat2_is_valid(...)` stability `stable`
- `proc mat2_clone` signature `proc mat2_clone(m: [int]) -> [int] { give m }` example `matrix.mat2_clone(...)` stability `stable`
- `proc mat2_m11` signature `proc mat2_m11(m: [int]) -> int { if m.len != 4 { give 0 } give m[0] }` example `matrix.mat2_m11(...)` stability `stable`
- `proc mat2_m12` signature `proc mat2_m12(m: [int]) -> int { if m.len != 4 { give 0 } give m[1] }` example `matrix.mat2_m12(...)` stability `stable`
- `proc mat2_m21` signature `proc mat2_m21(m: [int]) -> int { if m.len != 4 { give 0 } give m[2] }` example `matrix.mat2_m21(...)` stability `stable`
- `proc mat2_m22` signature `proc mat2_m22(m: [int]) -> int { if m.len != 4 { give 0 } give m[3] }` example `matrix.mat2_m22(...)` stability `stable`
- `proc mat2_row0` signature `proc mat2_row0(m: [int]) -> [int] { if m.len != 4 { give [] } give [m[0], m[1]] }` example `matrix.mat2_row0(...)` stability `stable`
- `proc mat2_row1` signature `proc mat2_row1(m: [int]) -> [int] { if m.len != 4 { give [] } give [m[2], m[3]] }` example `matrix.mat2_row1(...)` stability `stable`
- `proc mat2_col0` signature `proc mat2_col0(m: [int]) -> [int] { if m.len != 4 { give [] } give [m[0], m[2]] }` example `matrix.mat2_col0(...)` stability `stable`
- `proc mat2_col1` signature `proc mat2_col1(m: [int]) -> [int] { if m.len != 4 { give [] } give [m[1], m[3]] }` example `matrix.mat2_col1(...)` stability `stable`
- `proc mat2_equal` signature `proc mat2_equal(a: [int], b: [int]) -> bool { give a.len == 4 and b.len == 4 and a[0] == b[0] and a[1] == b[1] and a[2] == b[2] and a[3] == b[3] }` example `matrix.mat2_equal(...)` stability `stable`
- `proc mat2_is_zero` signature `proc mat2_is_zero(m: [int]) -> bool { give mat2_equal(m, mat2_zero()) }` example `matrix.mat2_is_zero(...)` stability `stable`
- `proc mat2_is_identity` signature `proc mat2_is_identity(m: [int]) -> bool { give mat2_equal(m, mat2_identity()) }` example `matrix.mat2_is_identity(...)` stability `stable`
- `proc mat2_is_diagonal` signature `proc mat2_is_diagonal(m: [int]) -> bool { give m.len == 4 and m[1] == 0 and m[2] == 0 }` example `matrix.mat2_is_diagonal(...)` stability `stable`
- `proc mat2_is_symmetric` signature `proc mat2_is_symmetric(m: [int]) -> bool { give m.len == 4 and m[1] == m[2] }` example `matrix.mat2_is_symmetric(...)` stability `stable`
- `proc mat2_is_upper_triangular` signature `proc mat2_is_upper_triangular(m: [int]) -> bool { give m.len == 4 and m[2] == 0 }` example `matrix.mat2_is_upper_triangular(...)` stability `stable`
- `proc mat2_is_lower_triangular` signature `proc mat2_is_lower_triangular(m: [int]) -> bool { give m.len == 4 and m[1] == 0 }` example `matrix.mat2_is_lower_triangular(...)` stability `stable`
- `proc mat2_add` signature `proc mat2_add(a: [int], b: [int]) -> [int] { if not mat2_is_valid(a) or not mat2_is_valid(b) { give [] } give [a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]] }` example `matrix.mat2_add(...)` stability `stable`
- `proc mat2_sub` signature `proc mat2_sub(a: [int], b: [int]) -> [int] { if not mat2_is_valid(a) or not mat2_is_valid(b) { give [] } give [a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]] }` example `matrix.mat2_sub(...)` stability `stable`
- `proc mat2_neg` signature `proc mat2_neg(m: [int]) -> [int] { if not mat2_is_valid(m) { give [] } give [0 - m[0], 0 - m[1], 0 - m[2], 0 - m[3]] }` example `matrix.mat2_neg(...)` stability `stable`
- `proc mat2_scale` signature `proc mat2_scale(m: [int], k: int) -> [int] { if not mat2_is_valid(m) { give [] } give [m[0] * k, m[1] * k, m[2] * k, m[3] * k] }` example `matrix.mat2_scale(...)` stability `stable`
- `proc mat2_hadamard` signature `proc mat2_hadamard(a: [int], b: [int]) -> [int] { if not mat2_is_valid(a) or not mat2_is_valid(b) { give [] } give [a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]] }` example `matrix.mat2_hadamard(...)` stability `stable`
- `proc mat2_mul` signature `proc mat2_mul(a: [int], b: [int]) -> [int] { if not mat2_is_valid(a) or not mat2_is_valid(b) { give [] } give [a[0] * b[0] + a[1] * b[2], a[0] * b[1] + a[1] * b[3], a[2] * b[0] + a[3] * b[2], a[2] * b[1] + a[3] * b[3]] }` example `matrix.mat2_mul(...)` stability `stable`
- `proc mat2_mul_vec2` signature `proc mat2_mul_vec2(m: [int], v: [int]) -> [int] { if not mat2_is_valid(m) or v.len != 2 { give [] } give [m[0] * v[0] + m[1] * v[1], m[2] * v[0] + m[3] * v[1]] }` example `matrix.mat2_mul_vec2(...)` stability `stable`
- `proc vec2_mul_mat2` signature `proc vec2_mul_mat2(v: [int], m: [int]) -> [int] { if not mat2_is_valid(m) or v.len != 2 { give [] } give [v[0] * m[0] + v[1] * m[2], v[0] * m[1] + v[1] * m[3]] }` example `matrix.vec2_mul_mat2(...)` stability `stable`
- `proc mat2_trace` signature `proc mat2_trace(m: [int]) -> int { if not mat2_is_valid(m) { give 0 } give m[0] + m[3] }` example `matrix.mat2_trace(...)` stability `stable`
- `proc mat2_det` signature `proc mat2_det(m: [int]) -> int { if not mat2_is_valid(m) { give 0 } give m[0] * m[3] - m[1] * m[2] }` example `matrix.mat2_det(...)` stability `stable`
- `proc mat2_transpose` signature `proc mat2_transpose(m: [int]) -> [int] { if not mat2_is_valid(m) { give [] } give [m[0], m[2], m[1], m[3]] }` example `matrix.mat2_transpose(...)` stability `stable`
- `proc mat2_adjugate` signature `proc mat2_adjugate(m: [int]) -> [int] { if not mat2_is_valid(m) { give [] } give [m[3], 0 - m[1], 0 - m[2], m[0]] }` example `matrix.mat2_adjugate(...)` stability `stable`
- `proc mat2_cofactor` signature `proc mat2_cofactor(m: [int]) -> [int] { give mat2_adjugate(m) }` example `matrix.mat2_cofactor(...)` stability `stable`
- `proc mat2_inv` signature `proc mat2_inv(m: [int]) -> [int] { let det: int = mat2_det(m) if det == 0 { give [] } let adj: [int] = mat2_adjugate(m) give [adj[0] / det, adj[1] / det, adj[2] / det, adj[3] / det] }` example `matrix.mat2_inv(...)` stability `stable`
- `proc mat2_has_inverse` signature `proc mat2_has_inverse(m: [int]) -> bool { give mat2_det(m) != 0 }` example `matrix.mat2_has_inverse(...)` stability `stable`
- `proc mat2_pow` signature `proc mat2_pow(m: [int], exp: int) -> [int] { if exp < 0 { give [] } let out: [int] = mat2_identity() let base: [int] = m let e: int = exp while e > 0 { if e % 2 == 1 { set out = mat2_mul(out, base) } set base = mat2_mul(base, base); set e = e / 2 } give out }` example `matrix.mat2_pow(...)` stability `stable`
- `proc mod_norm` signature `proc mod_norm(value: int, modulus: int) -> int { if modulus <= 0 { give 0 } let out: int = value % modulus if out < 0 { give out + modulus } give out }` example `matrix.mod_norm(...)` stability `stable`
- `proc mat2_mod_norm` signature `proc mat2_mod_norm(m: [int], modulus: int) -> [int] { if not mat2_is_valid(m) { give [] } give [mod_norm(m[0], modulus), mod_norm(m[1], modulus), mod_norm(m[2], modulus), mod_norm(m[3], modulus)] }` example `matrix.mat2_mod_norm(...)` stability `stable`
- `proc mat2_add_mod` signature `proc mat2_add_mod(a: [int], b: [int], modulus: int) -> [int] { give mat2_mod_norm(mat2_add(a, b), modulus) }` example `matrix.mat2_add_mod(...)` stability `stable`
- `proc mat2_sub_mod` signature `proc mat2_sub_mod(a: [int], b: [int], modulus: int) -> [int] { give mat2_mod_norm(mat2_sub(a, b), modulus) }` example `matrix.mat2_sub_mod(...)` stability `stable`
- `proc mat2_scale_mod` signature `proc mat2_scale_mod(m: [int], k: int, modulus: int) -> [int] { give mat2_mod_norm(mat2_scale(m, k), modulus) }` example `matrix.mat2_scale_mod(...)` stability `stable`
- `proc mat2_mul_mod` signature `proc mat2_mul_mod(a: [int], b: [int], modulus: int) -> [int] { give mat2_mod_norm(mat2_mul(a, b), modulus) }` example `matrix.mat2_mul_mod(...)` stability `stable`
- `proc mat2_det_mod` signature `proc mat2_det_mod(m: [int], modulus: int) -> int { give mod_norm(mat2_det(m), modulus) }` example `matrix.mat2_det_mod(...)` stability `stable`
- `proc mat2_inv_mod` signature `proc mat2_inv_mod(m: [int], modulus: int) -> [int] { give mat2_mod_norm(mat2_inv(m), modulus) }` example `matrix.mat2_inv_mod(...)` stability `stable`
- `proc mat2_pow_mod` signature `proc mat2_pow_mod(m: [int], exp: int, modulus: int) -> [int] { give mat2_mod_norm(mat2_pow(m, exp), modulus) }` example `matrix.mat2_pow_mod(...)` stability `stable`
- `proc mat2_fibonacci_matrix` signature `proc mat2_fibonacci_matrix() -> [int] { give [1, 1, 1, 0] }` example `matrix.mat2_fibonacci_matrix(...)` stability `stable`
- `proc mat2_fib` signature `proc mat2_fib(n: int) -> int { if n <= 0 { give 0 } if n == 1 { give 1 } let p: [int] = mat2_pow(mat2_fibonacci_matrix(), n - 1) give p[0] }` example `matrix.mat2_fib(...)` stability `stable`
- `proc mat2_fib_mod` signature `proc mat2_fib_mod(n: int, modulus: int) -> int { give mod_norm(mat2_fib(n), modulus) }` example `matrix.mat2_fib_mod(...)` stability `stable`
- `proc mat2_id` signature `proc mat2_id() -> [int] { give mat2_identity() }` example `matrix.mat2_id(...)` stability `stable`
- `proc matrix_version` signature `proc matrix_version() -> string { give "max-1" }` example `matrix.matrix_version(...)` stability `stable`
- `proc matrix_ready` signature `proc matrix_ready() -> bool { give true }` example `matrix.matrix_ready(...)` stability `stable`
- `proc matrix_selftest` signature `proc matrix_selftest() -> bool { let m: [int] = mat2(1, 2, 3, 4) let inv: [int] = mat2_inv(mat2(2, 0, 0, 2)) give matrix_ready() and matrix_version() == "max-1" and mat2_det(m) == -2 and mat2_trace(m) == 5 and mat2_mul_vec2(m, [1, 1])[0] == 3 and mat2_has_inverse(mat2(2, 0, 0, 2)) and inv.len == 4 and mat2_fib(8) == 21 }` example `matrix.matrix_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/modular.vitl`

Stability: `stable`

- `const MOD_INVALID` signature `const MOD_INVALID: int = 0` example `modular.MOD_INVALID` stability `stable`
- `form EgcdResult` signature `form EgcdResult {` example `modular.EgcdResult` stability `stable`
- `form Crt2Result` signature `form Crt2Result {` example `modular.Crt2Result` stability `stable`
- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `modular.abs_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `modular.min_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `modular.max_int(...)` stability `stable`
- `proc mod_is_valid` signature `proc mod_is_valid(modulus: int) -> bool {` example `modular.mod_is_valid(...)` stability `stable`
- `proc mod_norm` signature `proc mod_norm(value: int, modulus: int) -> int {` example `modular.mod_norm(...)` stability `stable`
- `proc mod_is_zero` signature `proc mod_is_zero(value: int, modulus: int) -> bool {` example `modular.mod_is_zero(...)` stability `stable`
- `proc mod_equal` signature `proc mod_equal(a: int, b: int, modulus: int) -> bool {` example `modular.mod_equal(...)` stability `stable`
- `proc are_congruent` signature `proc are_congruent(a: int, b: int, modulus: int) -> bool {` example `modular.are_congruent(...)` stability `stable`
- `proc mod_add` signature `proc mod_add(a: int, b: int, modulus: int) -> int {` example `modular.mod_add(...)` stability `stable`
- `proc mod_sub` signature `proc mod_sub(a: int, b: int, modulus: int) -> int {` example `modular.mod_sub(...)` stability `stable`
- `proc mod_neg` signature `proc mod_neg(a: int, modulus: int) -> int {` example `modular.mod_neg(...)` stability `stable`
- `proc mod_double` signature `proc mod_double(a: int, modulus: int) -> int {` example `modular.mod_double(...)` stability `stable`
- `proc mod_mul` signature `proc mod_mul(a: int, b: int, modulus: int) -> int {` example `modular.mod_mul(...)` stability `stable`
- `proc mod_mul_safe` signature `proc mod_mul_safe(a: int, b: int, modulus: int) -> int {` example `modular.mod_mul_safe(...)` stability `stable`
- `proc mod_square` signature `proc mod_square(a: int, modulus: int) -> int {` example `modular.mod_square(...)` stability `stable`
- `proc mod_cube` signature `proc mod_cube(a: int, modulus: int) -> int {` example `modular.mod_cube(...)` stability `stable`
- `proc extended_gcd` signature `proc extended_gcd(a: int, b: int) -> EgcdResult {` example `modular.extended_gcd(...)` stability `stable`
- `proc mod_is_unit` signature `proc mod_is_unit(a: int, modulus: int) -> bool {` example `modular.mod_is_unit(...)` stability `stable`
- `proc mod_has_inverse` signature `proc mod_has_inverse(a: int, modulus: int) -> bool {` example `modular.mod_has_inverse(...)` stability `stable`
- `proc mod_inv` signature `proc mod_inv(a: int, modulus: int) -> int {` example `modular.mod_inv(...)` stability `stable`
- `proc mod_div` signature `proc mod_div(a: int, b: int, modulus: int) -> int {` example `modular.mod_div(...)` stability `stable`
- `proc mod_half` signature `proc mod_half(a: int, modulus: int) -> int {` example `modular.mod_half(...)` stability `stable`
- `proc mod_pow` signature `proc mod_pow(a: int, exp: int, modulus: int) -> int {` example `modular.mod_pow(...)` stability `stable`
- `proc mod_pow2` signature `proc mod_pow2(exp: int, modulus: int) -> int {` example `modular.mod_pow2(...)` stability `stable`
- `proc mod_pow10` signature `proc mod_pow10(exp: int, modulus: int) -> int {` example `modular.mod_pow10(...)` stability `stable`
- `proc solve_linear_congruence` signature `proc solve_linear_congruence(a: int, b: int, modulus: int) -> int {` example `modular.solve_linear_congruence(...)` stability `stable`
- `proc crt2_full` signature `proc crt2_full(a1: int, m1: int, a2: int, m2: int) -> Crt2Result {` example `modular.crt2_full(...)` stability `stable`
- `proc crt2` signature `proc crt2(a1: int, m1: int, a2: int, m2: int) -> int {` example `modular.crt2(...)` stability `stable`
- `proc crt` signature `proc crt(residues: [int], moduli: [int]) -> int {` example `modular.crt(...)` stability `stable`
- `proc mod_factorial` signature `proc mod_factorial(n: int, modulus: int) -> int {` example `modular.mod_factorial(...)` stability `stable`
- `proc mod_permutations` signature `proc mod_permutations(n: int, k: int, modulus: int) -> int {` example `modular.mod_permutations(...)` stability `stable`
- `proc mod_combinations_prime` signature `proc mod_combinations_prime(n: int, k: int, modulus: int) -> int {` example `modular.mod_combinations_prime(...)` stability `stable`
- `proc mod_combinations` signature `proc mod_combinations(n: int, k: int, modulus: int) -> int {` example `modular.mod_combinations(...)` stability `stable`
- `proc mod_factorial_table` signature `proc mod_factorial_table(n: int, modulus: int) -> [int] {` example `modular.mod_factorial_table(...)` stability `stable`
- `proc mod_inverse_table` signature `proc mod_inverse_table(n: int, modulus: int) -> [int] {` example `modular.mod_inverse_table(...)` stability `stable`
- `proc mod_inverse_factorial_table` signature `proc mod_inverse_factorial_table(n: int, modulus: int) -> [int] {` example `modular.mod_inverse_factorial_table(...)` stability `stable`
- `proc mod_fibonacci` signature `proc mod_fibonacci(n: int, modulus: int) -> int {` example `modular.mod_fibonacci(...)` stability `stable`
- `proc mod_arithmetic_sum` signature `proc mod_arithmetic_sum(first: int, step: int, count0: int, modulus: int) -> int {` example `modular.mod_arithmetic_sum(...)` stability `stable`
- `proc mod_geometric_sum` signature `proc mod_geometric_sum(first: int, ratio: int, count0: int, modulus: int) -> int {` example `modular.mod_geometric_sum(...)` stability `stable`
- `proc mod_powers` signature `proc mod_powers(base: int, count0: int, modulus: int) -> [int] {` example `modular.mod_powers(...)` stability `stable`
- `proc mod_poly_eval` signature `proc mod_poly_eval(coeffs: [int], x: int, modulus: int) -> int {` example `modular.mod_poly_eval(...)` stability `stable`
- `proc mod_hash_step` signature `proc mod_hash_step(hash0: int, value: int, base: int, modulus: int) -> int {` example `modular.mod_hash_step(...)` stability `stable`
- `proc mod` signature `proc mod(value: int, modulus: int) -> int {` example `modular.mod(...)` stability `stable`
- `proc inv_mod` signature `proc inv_mod(value: int, modulus: int) -> int {` example `modular.inv_mod(...)` stability `stable`
- `proc modular_version` signature `proc modular_version() -> string {` example `modular.modular_version(...)` stability `stable`
- `proc modular_ready` signature `proc modular_ready() -> bool {` example `modular.modular_ready(...)` stability `stable`
- `proc modular_selftest` signature `proc modular_selftest() -> bool {` example `modular.modular_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/number_theory.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `number_theory.abs_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `number_theory.min_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `number_theory.max_int(...)` stability `stable`
- `proc reverse_copy` signature `proc reverse_copy(values: [int]) -> [int] {` example `number_theory.reverse_copy(...)` stability `stable`
- `proc is_even` signature `proc is_even(value: int) -> bool {` example `number_theory.is_even(...)` stability `stable`
- `proc is_odd` signature `proc is_odd(value: int) -> bool {` example `number_theory.is_odd(...)` stability `stable`
- `proc is_multiple_of` signature `proc is_multiple_of(value: int, divisor: int) -> bool {` example `number_theory.is_multiple_of(...)` stability `stable`
- `proc divides` signature `proc divides(divisor: int, value: int) -> bool {` example `number_theory.divides(...)` stability `stable`
- `proc gcd` signature `proc gcd(a: int, b: int) -> int {` example `number_theory.gcd(...)` stability `stable`
- `proc lcm` signature `proc lcm(a: int, b: int) -> int {` example `number_theory.lcm(...)` stability `stable`
- `proc are_coprime` signature `proc are_coprime(a: int, b: int) -> bool {` example `number_theory.are_coprime(...)` stability `stable`
- `proc gcd_many` signature `proc gcd_many(values: [int]) -> int {` example `number_theory.gcd_many(...)` stability `stable`
- `proc lcm_many` signature `proc lcm_many(values: [int]) -> int {` example `number_theory.lcm_many(...)` stability `stable`
- `proc is_prime` signature `proc is_prime(value: int) -> bool {` example `number_theory.is_prime(...)` stability `stable`
- `proc is_composite` signature `proc is_composite(value: int) -> bool {` example `number_theory.is_composite(...)` stability `stable`
- `proc next_prime` signature `proc next_prime(value: int) -> int {` example `number_theory.next_prime(...)` stability `stable`
- `proc prev_prime` signature `proc prev_prime(value: int) -> int {` example `number_theory.prev_prime(...)` stability `stable`
- `proc nth_prime` signature `proc nth_prime(index: int) -> int {` example `number_theory.nth_prime(...)` stability `stable`
- `proc primes_up_to` signature `proc primes_up_to(limit: int) -> [int] {` example `number_theory.primes_up_to(...)` stability `stable`
- `proc prime_count` signature `proc prime_count(limit: int) -> int {` example `number_theory.prime_count(...)` stability `stable`
- `proc prime_factors` signature `proc prime_factors(value: int) -> [int] {` example `number_theory.prime_factors(...)` stability `stable`
- `proc distinct_prime_factors` signature `proc distinct_prime_factors(value: int) -> [int] {` example `number_theory.distinct_prime_factors(...)` stability `stable`
- `proc prime_factor_count` signature `proc prime_factor_count(value: int) -> int {` example `number_theory.prime_factor_count(...)` stability `stable`
- `proc distinct_prime_factor_count` signature `proc distinct_prime_factor_count(value: int) -> int {` example `number_theory.distinct_prime_factor_count(...)` stability `stable`
- `proc smallest_prime_factor` signature `proc smallest_prime_factor(value: int) -> int {` example `number_theory.smallest_prime_factor(...)` stability `stable`
- `proc largest_prime_factor` signature `proc largest_prime_factor(value: int) -> int {` example `number_theory.largest_prime_factor(...)` stability `stable`
- `proc divisors` signature `proc divisors(value: int) -> [int] {` example `number_theory.divisors(...)` stability `stable`
- `proc proper_divisors` signature `proc proper_divisors(value: int) -> [int] {` example `number_theory.proper_divisors(...)` stability `stable`
- `proc divisors_count` signature `proc divisors_count(value: int) -> int {` example `number_theory.divisors_count(...)` stability `stable`
- `proc sum_of_divisors` signature `proc sum_of_divisors(value: int) -> int {` example `number_theory.sum_of_divisors(...)` stability `stable`
- `proc proper_divisors_sum` signature `proc proper_divisors_sum(value: int) -> int {` example `number_theory.proper_divisors_sum(...)` stability `stable`
- `proc totient` signature `proc totient(value: int) -> int {` example `number_theory.totient(...)` stability `stable`
- `proc mobius` signature `proc mobius(value: int) -> int {` example `number_theory.mobius(...)` stability `stable`
- `proc is_perfect_number` signature `proc is_perfect_number(value: int) -> bool {` example `number_theory.is_perfect_number(...)` stability `stable`
- `proc is_abundant_number` signature `proc is_abundant_number(value: int) -> bool {` example `number_theory.is_abundant_number(...)` stability `stable`
- `proc is_deficient_number` signature `proc is_deficient_number(value: int) -> bool {` example `number_theory.is_deficient_number(...)` stability `stable`
- `proc coprime` signature `proc coprime(a: int, b: int) -> bool {` example `number_theory.coprime(...)` stability `stable`
- `proc number_theory_version` signature `proc number_theory_version() -> string {` example `number_theory.number_theory_version(...)` stability `stable`
- `proc number_theory_ready` signature `proc number_theory_ready() -> bool {` example `number_theory.number_theory_ready(...)` stability `stable`
- `proc number_theory_selftest` signature `proc number_theory_selftest() -> bool {` example `number_theory.number_theory_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/powers.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `powers.abs_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `powers.min_int(...)` stability `stable`
- `proc gcd_int` signature `proc gcd_int(a: int, b: int) -> int {` example `powers.gcd_int(...)` stability `stable`
- `proc pow_int` signature `proc pow_int(base: int, exponent: int) -> int {` example `powers.pow_int(...)` stability `stable`
- `proc pow2` signature `proc pow2(exponent: int) -> int {` example `powers.pow2(...)` stability `stable`
- `proc pow10` signature `proc pow10(exponent: int) -> int {` example `powers.pow10(...)` stability `stable`
- `proc square` signature `proc square(value: int) -> int {` example `powers.square(...)` stability `stable`
- `proc cube` signature `proc cube(value: int) -> int {` example `powers.cube(...)` stability `stable`
- `proc factorial` signature `proc factorial(value: int) -> int {` example `powers.factorial(...)` stability `stable`
- `proc double_factorial` signature `proc double_factorial(value: int) -> int {` example `powers.double_factorial(...)` stability `stable`
- `proc falling_factorial` signature `proc falling_factorial(n: int, k: int) -> int {` example `powers.falling_factorial(...)` stability `stable`
- `proc rising_factorial` signature `proc rising_factorial(n: int, k: int) -> int {` example `powers.rising_factorial(...)` stability `stable`
- `proc triangular` signature `proc triangular(index: int) -> int {` example `powers.triangular(...)` stability `stable`
- `proc fibonacci` signature `proc fibonacci(index: int) -> int {` example `powers.fibonacci(...)` stability `stable`
- `proc fibonacci_sum` signature `proc fibonacci_sum(count: int) -> int {` example `powers.fibonacci_sum(...)` stability `stable`
- `proc fibonacci_series` signature `proc fibonacci_series(count: int) -> [int] {` example `powers.fibonacci_series(...)` stability `stable`
- `proc is_square_local` signature `proc is_square_local(value: int) -> bool {` example `powers.is_square_local(...)` stability `stable`
- `proc is_fibonacci` signature `proc is_fibonacci(value: int) -> bool {` example `powers.is_fibonacci(...)` stability `stable`
- `proc binomial` signature `proc binomial(n: int, k: int) -> int {` example `powers.binomial(...)` stability `stable`
- `proc combinations` signature `proc combinations(n: int, k: int) -> int {` example `powers.combinations(...)` stability `stable`
- `proc choose` signature `proc choose(n: int, k: int) -> int {` example `powers.choose(...)` stability `stable`
- `proc permutations` signature `proc permutations(n: int, k: int) -> int {` example `powers.permutations(...)` stability `stable`
- `proc arrangements` signature `proc arrangements(n: int, k: int) -> int {` example `powers.arrangements(...)` stability `stable`
- `proc combinations_with_repetition` signature `proc combinations_with_repetition(n: int, k: int) -> int {` example `powers.combinations_with_repetition(...)` stability `stable`
- `proc fib` signature `proc fib(index: int) -> int {` example `powers.fib(...)` stability `stable`
- `proc powers_version` signature `proc powers_version() -> string {` example `powers.powers_version(...)` stability `stable`
- `proc powers_ready` signature `proc powers_ready() -> bool {` example `powers.powers_ready(...)` stability `stable`
- `proc powers_selftest` signature `proc powers_selftest() -> bool {` example `powers.powers_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/probability.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `probability.abs_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `probability.min_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `probability.max_int(...)` stability `stable`
- `proc gcd_int` signature `proc gcd_int(a: int, b: int) -> int {` example `probability.gcd_int(...)` stability `stable`
- `proc valid_n_k` signature `proc valid_n_k(n: int, k: int) -> bool {` example `probability.valid_n_k(...)` stability `stable`
- `proc clamp_non_negative` signature `proc clamp_non_negative(value: int) -> int {` example `probability.clamp_non_negative(...)` stability `stable`
- `proc factorial` signature `proc factorial(value: int) -> int {` example `probability.factorial(...)` stability `stable`
- `proc double_factorial` signature `proc double_factorial(value: int) -> int {` example `probability.double_factorial(...)` stability `stable`
- `proc falling_factorial` signature `proc falling_factorial(n: int, k: int) -> int {` example `probability.falling_factorial(...)` stability `stable`
- `proc rising_factorial` signature `proc rising_factorial(n: int, k: int) -> int {` example `probability.rising_factorial(...)` stability `stable`
- `proc permutations` signature `proc permutations(n: int, k: int) -> int {` example `probability.permutations(...)` stability `stable`
- `proc arrangements` signature `proc arrangements(n: int, k: int) -> int {` example `probability.arrangements(...)` stability `stable`
- `proc permutations_with_repetition` signature `proc permutations_with_repetition(n: int, k: int) -> int {` example `probability.permutations_with_repetition(...)` stability `stable`
- `proc combinations` signature `proc combinations(n: int, k: int) -> int {` example `probability.combinations(...)` stability `stable`
- `proc choose` signature `proc choose(n: int, k: int) -> int {` example `probability.choose(...)` stability `stable`
- `proc combinations_with_repetition` signature `proc combinations_with_repetition(n: int, k: int) -> int {` example `probability.combinations_with_repetition(...)` stability `stable`
- `proc multiset_combinations` signature `proc multiset_combinations(n: int, k: int) -> int {` example `probability.multiset_combinations(...)` stability `stable`
- `proc pairings` signature `proc pairings(total: int) -> int {` example `probability.pairings(...)` stability `stable`
- `proc unordered_pairs` signature `proc unordered_pairs(total: int) -> int {` example `probability.unordered_pairs(...)` stability `stable`
- `proc ordered_pairs` signature `proc ordered_pairs(total: int) -> int {` example `probability.ordered_pairs(...)` stability `stable`
- `proc derangements` signature `proc derangements(n: int) -> int {` example `probability.derangements(...)` stability `stable`
- `proc ratio_f64` signature `proc ratio_f64(part: int, total: int) -> f64 {` example `probability.ratio_f64(...)` stability `stable`
- `proc percentage_f64` signature `proc percentage_f64(part: int, total: int) -> f64 {` example `probability.percentage_f64(...)` stability `stable`
- `proc probability` signature `proc probability(part: int, total: int) -> f64 {` example `probability.probability(...)` stability `stable`
- `proc probability_percent` signature `proc probability_percent(part: int, total: int) -> int {` example `probability.probability_percent(...)` stability `stable`
- `proc probability_percent_f64` signature `proc probability_percent_f64(part: int, total: int) -> f64 {` example `probability.probability_percent_f64(...)` stability `stable`
- `proc permille_f64` signature `proc permille_f64(part: int, total: int) -> f64 {` example `probability.permille_f64(...)` stability `stable`
- `proc basis_points_f64` signature `proc basis_points_f64(part: int, total: int) -> f64 {` example `probability.basis_points_f64(...)` stability `stable`
- `proc complement_probability` signature `proc complement_probability(part: int, total: int) -> f64 {` example `probability.complement_probability(...)` stability `stable`
- `proc complement_percent` signature `proc complement_percent(part: int, total: int) -> int {` example `probability.complement_percent(...)` stability `stable`
- `proc success_rate_f64` signature `proc success_rate_f64(successes: int, total: int) -> f64 {` example `probability.success_rate_f64(...)` stability `stable`
- `proc failure_rate_f64` signature `proc failure_rate_f64(successes: int, total: int) -> f64 {` example `probability.failure_rate_f64(...)` stability `stable`
- `proc success_rate_percent` signature `proc success_rate_percent(successes: int, total: int) -> int {` example `probability.success_rate_percent(...)` stability `stable`
- `proc failure_rate_percent` signature `proc failure_rate_percent(successes: int, total: int) -> int {` example `probability.failure_rate_percent(...)` stability `stable`
- `proc odds_f64` signature `proc odds_f64(successes: int, failures: int) -> f64 {` example `probability.odds_f64(...)` stability `stable`
- `proc odds_percent` signature `proc odds_percent(successes: int, failures: int) -> int {` example `probability.odds_percent(...)` stability `stable`
- `proc odds_ratio` signature `proc odds_ratio(successes: int, failures: int) -> int {` example `probability.odds_ratio(...)` stability `stable`
- `proc odds_ratio_f64` signature `proc odds_ratio_f64(successes: int, failures: int) -> f64 {` example `probability.odds_ratio_f64(...)` stability `stable`
- `proc success_failure_ratio_percent` signature `proc success_failure_ratio_percent(successes: int, failures: int) -> int {` example `probability.success_failure_ratio_percent(...)` stability `stable`
- `proc relative_risk_f64` signature `proc relative_risk_f64(exposed_successes: int, exposed_total: int, control_successes: int, control_total: int) -> f64 {` example `probability.relative_risk_f64(...)` stability `stable`
- `proc odds_ratio_2x2` signature `proc odds_ratio_2x2(a: int, b: int, c: int, d: int) -> f64 {` example `probability.odds_ratio_2x2(...)` stability `stable`
- `proc favorable_probability` signature `proc favorable_probability(favorable: int, total: int) -> f64 {` example `probability.favorable_probability(...)` stability `stable`
- `proc equally_likely_probability` signature `proc equally_likely_probability(favorable: int, possible: int) -> f64 {` example `probability.equally_likely_probability(...)` stability `stable`
- `proc union_probability_disjoint` signature `proc union_probability_disjoint(p_a: f64, p_b: f64) -> f64 {` example `probability.union_probability_disjoint(...)` stability `stable`
- `proc union_probability` signature `proc union_probability(p_a: f64, p_b: f64, p_intersection: f64) -> f64 {` example `probability.union_probability(...)` stability `stable`
- `proc intersection_independent` signature `proc intersection_independent(p_a: f64, p_b: f64) -> f64 {` example `probability.intersection_independent(...)` stability `stable`
- `proc complement_from_probability` signature `proc complement_from_probability(p: f64) -> f64 {` example `probability.complement_from_probability(...)` stability `stable`
- `proc conditional_probability` signature `proc conditional_probability(intersection: f64, given_event: f64) -> f64 {` example `probability.conditional_probability(...)` stability `stable`
- `proc expected_value_binary` signature `proc expected_value_binary(success_probability: f64, success_value: f64, failure_value: f64) -> f64 {` example `probability.expected_value_binary(...)` stability `stable`
- `proc binomial_pmf` signature `proc binomial_pmf(trials: int, successes: int, p: f64) -> f64 {` example `probability.binomial_pmf(...)` stability `stable`
- `proc geometric_pmf` signature `proc geometric_pmf(trial_index: int, p: f64) -> f64 {` example `probability.geometric_pmf(...)` stability `stable`
- `proc hypergeometric_pmf` signature `proc hypergeometric_pmf(population: int, success_population: int, draws: int, observed_successes: int) -> f64 {` example `probability.hypergeometric_pmf(...)` stability `stable`
- `proc percent` signature `proc percent(part: int, total: int) -> int {` example `probability.percent(...)` stability `stable`
- `proc percent_f64` signature `proc percent_f64(part: int, total: int) -> f64 {` example `probability.percent_f64(...)` stability `stable`
- `proc choose_probability` signature `proc choose_probability(favorable: int, total: int) -> f64 {` example `probability.choose_probability(...)` stability `stable`
- `proc probability_version` signature `proc probability_version() -> string {` example `probability.probability_version(...)` stability `stable`
- `proc probability_ready` signature `proc probability_ready() -> bool {` example `probability.probability_ready(...)` stability `stable`
- `proc probability_selftest` signature `proc probability_selftest() -> bool {` example `probability.probability_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/roots.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(value: int) -> int {` example `roots.abs_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `roots.max_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `roots.min_int(...)` stability `stable`
- `proc square` signature `proc square(value: int) -> int {` example `roots.square(...)` stability `stable`
- `proc cube` signature `proc cube(value: int) -> int {` example `roots.cube(...)` stability `stable`
- `proc square_le` signature `proc square_le(candidate: int, limit: int) -> bool {` example `roots.square_le(...)` stability `stable`
- `proc square_eq` signature `proc square_eq(candidate: int, value: int) -> bool {` example `roots.square_eq(...)` stability `stable`
- `proc cube_le_non_negative` signature `proc cube_le_non_negative(candidate: int, limit: int) -> bool {` example `roots.cube_le_non_negative(...)` stability `stable`
- `proc cube_eq_non_negative` signature `proc cube_eq_non_negative(candidate: int, value: int) -> bool {` example `roots.cube_eq_non_negative(...)` stability `stable`
- `proc pow_int` signature `proc pow_int(base: int, exponent: int) -> int {` example `roots.pow_int(...)` stability `stable`
- `proc pow_le_non_negative` signature `proc pow_le_non_negative(base: int, exponent: int, limit: int) -> bool {` example `roots.pow_le_non_negative(...)` stability `stable`
- `proc pow_eq_non_negative` signature `proc pow_eq_non_negative(base: int, exponent: int, value: int) -> bool {` example `roots.pow_eq_non_negative(...)` stability `stable`
- `proc sqrt_floor` signature `proc sqrt_floor(value: int) -> int {` example `roots.sqrt_floor(...)` stability `stable`
- `proc sqrt_ceil` signature `proc sqrt_ceil(value: int) -> int {` example `roots.sqrt_ceil(...)` stability `stable`
- `proc sqrt_round` signature `proc sqrt_round(value: int) -> int {` example `roots.sqrt_round(...)` stability `stable`
- `proc is_square` signature `proc is_square(value: int) -> bool {` example `roots.is_square(...)` stability `stable`
- `proc prev_square` signature `proc prev_square(value: int) -> int {` example `roots.prev_square(...)` stability `stable`
- `proc next_square` signature `proc next_square(value: int) -> int {` example `roots.next_square(...)` stability `stable`
- `proc cube_root_floor_non_negative` signature `proc cube_root_floor_non_negative(value: int) -> int {` example `roots.cube_root_floor_non_negative(...)` stability `stable`
- `proc cube_root_ceil_non_negative` signature `proc cube_root_ceil_non_negative(value: int) -> int {` example `roots.cube_root_ceil_non_negative(...)` stability `stable`
- `proc cube_root_floor` signature `proc cube_root_floor(value: int) -> int {` example `roots.cube_root_floor(...)` stability `stable`
- `proc cube_root_ceil` signature `proc cube_root_ceil(value: int) -> int {` example `roots.cube_root_ceil(...)` stability `stable`
- `proc cube_root_round` signature `proc cube_root_round(value: int) -> int {` example `roots.cube_root_round(...)` stability `stable`
- `proc is_cube` signature `proc is_cube(value: int) -> bool {` example `roots.is_cube(...)` stability `stable`
- `proc prev_cube` signature `proc prev_cube(value: int) -> int {` example `roots.prev_cube(...)` stability `stable`
- `proc next_cube` signature `proc next_cube(value: int) -> int {` example `roots.next_cube(...)` stability `stable`
- `proc iroot_floor_non_negative` signature `proc iroot_floor_non_negative(value: int, degree: int) -> int {` example `roots.iroot_floor_non_negative(...)` stability `stable`
- `proc iroot_floor` signature `proc iroot_floor(value: int, degree: int) -> int {` example `roots.iroot_floor(...)` stability `stable`
- `proc iroot_ceil` signature `proc iroot_ceil(value: int, degree: int) -> int {` example `roots.iroot_ceil(...)` stability `stable`
- `proc iroot_round` signature `proc iroot_round(value: int, degree: int) -> int {` example `roots.iroot_round(...)` stability `stable`
- `proc is_perfect_power` signature `proc is_perfect_power(value: int, degree: int) -> bool {` example `roots.is_perfect_power(...)` stability `stable`
- `proc ilog2_floor` signature `proc ilog2_floor(value: int) -> int {` example `roots.ilog2_floor(...)` stability `stable`
- `proc ilog2_ceil` signature `proc ilog2_ceil(value: int) -> int {` example `roots.ilog2_ceil(...)` stability `stable`
- `proc ilog10_floor` signature `proc ilog10_floor(value: int) -> int {` example `roots.ilog10_floor(...)` stability `stable`
- `proc ilog10_ceil` signature `proc ilog10_ceil(value: int) -> int {` example `roots.ilog10_ceil(...)` stability `stable`
- `proc ilog_floor` signature `proc ilog_floor(value: int, base: int) -> int {` example `roots.ilog_floor(...)` stability `stable`
- `proc ilog_ceil` signature `proc ilog_ceil(value: int, base: int) -> int {` example `roots.ilog_ceil(...)` stability `stable`
- `proc is_power_of_two` signature `proc is_power_of_two(value: int) -> bool {` example `roots.is_power_of_two(...)` stability `stable`
- `proc is_power_of_ten` signature `proc is_power_of_ten(value: int) -> bool {` example `roots.is_power_of_ten(...)` stability `stable`
- `proc is_power` signature `proc is_power(value: int, base: int) -> bool {` example `roots.is_power(...)` stability `stable`
- `proc decimal_digits` signature `proc decimal_digits(value: int) -> int {` example `roots.decimal_digits(...)` stability `stable`
- `proc binary_digits` signature `proc binary_digits(value: int) -> int {` example `roots.binary_digits(...)` stability `stable`
- `proc distance_to_prev_square` signature `proc distance_to_prev_square(value: int) -> int {` example `roots.distance_to_prev_square(...)` stability `stable`
- `proc distance_to_next_square` signature `proc distance_to_next_square(value: int) -> int {` example `roots.distance_to_next_square(...)` stability `stable`
- `proc distance_to_nearest_square` signature `proc distance_to_nearest_square(value: int) -> int {` example `roots.distance_to_nearest_square(...)` stability `stable`
- `proc distance_to_prev_cube` signature `proc distance_to_prev_cube(value: int) -> int {` example `roots.distance_to_prev_cube(...)` stability `stable`
- `proc distance_to_next_cube` signature `proc distance_to_next_cube(value: int) -> int {` example `roots.distance_to_next_cube(...)` stability `stable`
- `proc distance_to_nearest_cube` signature `proc distance_to_nearest_cube(value: int) -> int {` example `roots.distance_to_nearest_cube(...)` stability `stable`
- `proc is_perfect_square` signature `proc is_perfect_square(value: int) -> bool {` example `roots.is_perfect_square(...)` stability `stable`
- `proc is_perfect_cube` signature `proc is_perfect_cube(value: int) -> bool {` example `roots.is_perfect_cube(...)` stability `stable`
- `proc roots_version` signature `proc roots_version() -> string {` example `roots.roots_version(...)` stability `stable`
- `proc roots_ready` signature `proc roots_ready() -> bool {` example `roots.roots_ready(...)` stability `stable`
- `proc roots_selftest` signature `proc roots_selftest() -> bool {` example `roots.roots_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/sequences.vitl`

Stability: `stable`

- `proc abs_int` signature `proc abs_int(x: int) -> int {` example `sequences.abs_int(...)` stability `stable`
- `proc max_int` signature `proc max_int(a: int, b: int) -> int {` example `sequences.max_int(...)` stability `stable`
- `proc min_int` signature `proc min_int(a: int, b: int) -> int {` example `sequences.min_int(...)` stability `stable`
- `proc clamp_non_negative` signature `proc clamp_non_negative(x: int) -> int {` example `sequences.clamp_non_negative(...)` stability `stable`
- `proc pow_int` signature `proc pow_int(base: int, exponent: int) -> int {` example `sequences.pow_int(...)` stability `stable`
- `proc integer_sqrt` signature `proc integer_sqrt(value: int) -> int {` example `sequences.integer_sqrt(...)` stability `stable`
- `proc is_perfect_square` signature `proc is_perfect_square(value: int) -> bool {` example `sequences.is_perfect_square(...)` stability `stable`
- `proc arithmetic_term` signature `proc arithmetic_term(first: int, step: int, index: int) -> int {` example `sequences.arithmetic_term(...)` stability `stable`
- `proc arithmetic_last` signature `proc arithmetic_last(first: int, step: int, count: int) -> int {` example `sequences.arithmetic_last(...)` stability `stable`
- `proc arithmetic_sum` signature `proc arithmetic_sum(first: int, step: int, count: int) -> int {` example `sequences.arithmetic_sum(...)` stability `stable`
- `proc arithmetic_mean` signature `proc arithmetic_mean(first: int, step: int, count: int) -> int {` example `sequences.arithmetic_mean(...)` stability `stable`
- `proc arithmetic_series` signature `proc arithmetic_series(first: int, step: int, count: int) -> [int] {` example `sequences.arithmetic_series(...)` stability `stable`
- `proc geometric_term` signature `proc geometric_term(first: int, ratio: int, index: int) -> int {` example `sequences.geometric_term(...)` stability `stable`
- `proc geometric_last` signature `proc geometric_last(first: int, ratio: int, count: int) -> int {` example `sequences.geometric_last(...)` stability `stable`
- `proc geometric_sum` signature `proc geometric_sum(first: int, ratio: int, count: int) -> int {` example `sequences.geometric_sum(...)` stability `stable`
- `proc geometric_series` signature `proc geometric_series(first: int, ratio: int, count: int) -> [int] {` example `sequences.geometric_series(...)` stability `stable`
- `proc triangular_term` signature `proc triangular_term(index: int) -> int {` example `sequences.triangular_term(...)` stability `stable`
- `proc triangular_sum` signature `proc triangular_sum(count: int) -> int {` example `sequences.triangular_sum(...)` stability `stable`
- `proc triangular_series` signature `proc triangular_series(count: int) -> [int] {` example `sequences.triangular_series(...)` stability `stable`
- `proc is_triangular` signature `proc is_triangular(value: int) -> bool {` example `sequences.is_triangular(...)` stability `stable`
- `proc square_term` signature `proc square_term(index: int) -> int {` example `sequences.square_term(...)` stability `stable`
- `proc square_series` signature `proc square_series(count: int) -> [int] {` example `sequences.square_series(...)` stability `stable`
- `proc cube_term` signature `proc cube_term(index: int) -> int {` example `sequences.cube_term(...)` stability `stable`
- `proc cube_series` signature `proc cube_series(count: int) -> [int] {` example `sequences.cube_series(...)` stability `stable`
- `proc polygonal_term` signature `proc polygonal_term(sides: int, index: int) -> int {` example `sequences.polygonal_term(...)` stability `stable`
- `proc pentagonal_term` signature `proc pentagonal_term(index: int) -> int {` example `sequences.pentagonal_term(...)` stability `stable`
- `proc hexagonal_term` signature `proc hexagonal_term(index: int) -> int {` example `sequences.hexagonal_term(...)` stability `stable`
- `proc polygonal_series` signature `proc polygonal_series(sides: int, count: int) -> [int] {` example `sequences.polygonal_series(...)` stability `stable`
- `proc fibonacci_term` signature `proc fibonacci_term(index: int) -> int {` example `sequences.fibonacci_term(...)` stability `stable`
- `proc fibonacci_sum` signature `proc fibonacci_sum(count: int) -> int {` example `sequences.fibonacci_sum(...)` stability `stable`
- `proc fibonacci_series` signature `proc fibonacci_series(count: int) -> [int] {` example `sequences.fibonacci_series(...)` stability `stable`
- `proc is_fibonacci` signature `proc is_fibonacci(value: int) -> bool {` example `sequences.is_fibonacci(...)` stability `stable`
- `proc lucas_term` signature `proc lucas_term(index: int) -> int {` example `sequences.lucas_term(...)` stability `stable`
- `proc lucas_series` signature `proc lucas_series(count: int) -> [int] {` example `sequences.lucas_series(...)` stability `stable`
- `proc pell_term` signature `proc pell_term(index: int) -> int {` example `sequences.pell_term(...)` stability `stable`
- `proc pell_series` signature `proc pell_series(count: int) -> [int] {` example `sequences.pell_series(...)` stability `stable`
- `proc factorial` signature `proc factorial(value: int) -> int {` example `sequences.factorial(...)` stability `stable`
- `proc double_factorial` signature `proc double_factorial(value: int) -> int {` example `sequences.double_factorial(...)` stability `stable`
- `proc catalan_term` signature `proc catalan_term(index: int) -> int {` example `sequences.catalan_term(...)` stability `stable`
- `proc catalan_series` signature `proc catalan_series(count: int) -> [int] {` example `sequences.catalan_series(...)` stability `stable`
- `proc harmonic_numerator_term` signature `proc harmonic_numerator_term(index: int) -> int {` example `sequences.harmonic_numerator_term(...)` stability `stable`
- `proc harmonic_denominator_term` signature `proc harmonic_denominator_term(index: int) -> int {` example `sequences.harmonic_denominator_term(...)` stability `stable`
- `proc harmonic_denominators` signature `proc harmonic_denominators(count: int) -> [int] {` example `sequences.harmonic_denominators(...)` stability `stable`
- `proc alternating_sign` signature `proc alternating_sign(index: int) -> int {` example `sequences.alternating_sign(...)` stability `stable`
- `proc alternating_arithmetic_term` signature `proc alternating_arithmetic_term(first: int, step: int, index: int) -> int {` example `sequences.alternating_arithmetic_term(...)` stability `stable`
- `proc alternating_geometric_term` signature `proc alternating_geometric_term(first: int, ratio: int, index: int) -> int {` example `sequences.alternating_geometric_term(...)` stability `stable`
- `proc first_arithmetic_ge` signature `proc first_arithmetic_ge(first: int, step: int, target: int) -> int {` example `sequences.first_arithmetic_ge(...)` stability `stable`
- `proc first_geometric_ge` signature `proc first_geometric_ge(first: int, ratio: int, target: int) -> int {` example `sequences.first_geometric_ge(...)` stability `stable`
- `proc fib` signature `proc fib(index: int) -> int {` example `sequences.fib(...)` stability `stable`
- `proc lucas` signature `proc lucas(index: int) -> int {` example `sequences.lucas(...)` stability `stable`
- `proc triangular` signature `proc triangular(index: int) -> int {` example `sequences.triangular(...)` stability `stable`
- `proc square` signature `proc square(index: int) -> int {` example `sequences.square(...)` stability `stable`
- `proc cube` signature `proc cube(index: int) -> int {` example `sequences.cube(...)` stability `stable`
- `proc sequences_version` signature `proc sequences_version() -> string {` example `sequences.sequences_version(...)` stability `stable`
- `proc sequences_ready` signature `proc sequences_ready() -> bool {` example `sequences.sequences_ready(...)` stability `stable`
- `proc sequences_selftest` signature `proc sequences_selftest() -> bool {` example `sequences.sequences_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/sort.vitl`

Stability: `stable`

- `const INSERTION_THRESHOLD` signature `const INSERTION_THRESHOLD: int = 16` example `sort.INSERTION_THRESHOLD` stability `stable`
- `proc copy_range` signature `proc copy_range(values: [int], start: int, end: int) -> [int] {` example `sort.copy_range(...)` stability `stable`
- `proc reverse_copy` signature `proc reverse_copy(values: [int]) -> [int] {` example `sort.reverse_copy(...)` stability `stable`
- `proc swap` signature `proc swap(values: [int], i: int, j: int) -> [int] {` example `sort.swap(...)` stability `stable`
- `proc floor_log2_int` signature `proc floor_log2_int(value: int) -> int {` example `sort.floor_log2_int(...)` stability `stable`
- `proc median3_index` signature `proc median3_index(values: [int], a: int, b: int, c: int) -> int {` example `sort.median3_index(...)` stability `stable`
- `proc is_sorted` signature `proc is_sorted(values: [int]) -> bool {` example `sort.is_sorted(...)` stability `stable`
- `proc is_strictly_sorted` signature `proc is_strictly_sorted(values: [int]) -> bool {` example `sort.is_strictly_sorted(...)` stability `stable`
- `proc insertion_sort_range` signature `proc insertion_sort_range(values: [int], start: int, end: int) -> [int] {` example `sort.insertion_sort_range(...)` stability `stable`
- `proc insertion_sort_inplace` signature `proc insertion_sort_inplace(values: [int]) -> [int] {` example `sort.insertion_sort_inplace(...)` stability `stable`
- `proc insertion_sort` signature `proc insertion_sort(values: [int]) -> [int] {` example `sort.insertion_sort(...)` stability `stable`
- `proc sift_down` signature `proc sift_down(values: [int], start: int, end: int) -> [int] {` example `sort.sift_down(...)` stability `stable`
- `proc heapify` signature `proc heapify(values: [int]) -> [int] {` example `sort.heapify(...)` stability `stable`
- `proc sort_inplace` signature `proc sort_inplace(values: [int]) -> [int] {` example `sort.sort_inplace(...)` stability `stable`
- `proc heapsort_inplace` signature `proc heapsort_inplace(values: [int]) -> [int] {` example `sort.heapsort_inplace(...)` stability `stable`
- `proc heapsort` signature `proc heapsort(values: [int]) -> [int] {` example `sort.heapsort(...)` stability `stable`
- `proc partition` signature `proc partition(values: [int], pivot: int) -> [[int]] {` example `sort.partition(...)` stability `stable`
- `proc heapsort_range` signature `proc heapsort_range(values: [int], start: int, end: int) -> [int] {` example `sort.heapsort_range(...)` stability `stable`
- `proc introsort_rec` signature `proc introsort_rec(values: [int]) -> [int] {` example `sort.introsort_rec(...)` stability `stable`
- `proc quicksort_inplace` signature `proc quicksort_inplace(values: [int]) -> [int] {` example `sort.quicksort_inplace(...)` stability `stable`
- `proc quicksort` signature `proc quicksort(values: [int]) -> [int] {` example `sort.quicksort(...)` stability `stable`
- `proc introsort` signature `proc introsort(values: [int]) -> [int] {` example `sort.introsort(...)` stability `stable`
- `proc merge` signature `proc merge(a: [int], b: [int]) -> [int] {` example `sort.merge(...)` stability `stable`
- `proc mergesort` signature `proc mergesort(values: [int]) -> [int] {` example `sort.mergesort(...)` stability `stable`
- `proc stable_sort` signature `proc stable_sort(values: [int]) -> [int] {` example `sort.stable_sort(...)` stability `stable`
- `proc reverse` signature `proc reverse(values: [int]) -> [int] {` example `sort.reverse(...)` stability `stable`
- `proc reverse_inplace` signature `proc reverse_inplace(values: [int]) -> [int] {` example `sort.reverse_inplace(...)` stability `stable`
- `proc sort_desc` signature `proc sort_desc(values: [int]) -> [int] {` example `sort.sort_desc(...)` stability `stable`
- `proc sort_desc_inplace` signature `proc sort_desc_inplace(values: [int]) -> [int] {` example `sort.sort_desc_inplace(...)` stability `stable`
- `proc nth_element` signature `proc nth_element(values: [int], index: int) -> int {` example `sort.nth_element(...)` stability `stable`
- `proc partial_sort` signature `proc partial_sort(values: [int], count0: int) -> [int] {` example `sort.partial_sort(...)` stability `stable`
- `proc top_k` signature `proc top_k(values: [int], k: int) -> [int] {` example `sort.top_k(...)` stability `stable`
- `proc bottom_k` signature `proc bottom_k(values: [int], k: int) -> [int] {` example `sort.bottom_k(...)` stability `stable`
- `proc lower_bound` signature `proc lower_bound(values: [int], needle: int) -> int {` example `sort.lower_bound(...)` stability `stable`
- `proc upper_bound` signature `proc upper_bound(values: [int], needle: int) -> int {` example `sort.upper_bound(...)` stability `stable`
- `proc binary_search` signature `proc binary_search(values: [int], needle: int) -> int {` example `sort.binary_search(...)` stability `stable`
- `proc contains_sorted` signature `proc contains_sorted(values: [int], needle: int) -> bool {` example `sort.contains_sorted(...)` stability `stable`
- `proc sort_int` signature `proc sort_int(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort_int(...)` stability `stable`
- `proc sort_i32` signature `proc sort_i32(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort_i32(...)` stability `stable`
- `proc sort_i64` signature `proc sort_i64(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort_i64(...)` stability `stable`
- `proc sort_u32` signature `proc sort_u32(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort_u32(...)` stability `stable`
- `proc sort_u64` signature `proc sort_u64(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort_u64(...)` stability `stable`
- `proc sort_f64` signature `proc sort_f64(values: [f64]) -> [f64] {` example `sort.sort_f64(...)` stability `stable`
- `proc stable_sort_int` signature `proc stable_sort_int(values: [int]) -> [int] { give stable_sort(values) }` example `sort.stable_sort_int(...)` stability `stable`
- `proc stable_sort_f64` signature `proc stable_sort_f64(values: [f64]) -> [f64] { give sort_f64(values) }` example `sort.stable_sort_f64(...)` stability `stable`
- `proc sort` signature `proc sort(values: [int]) -> [int] { give sort_inplace(values) }` example `sort.sort(...)` stability `stable`
- `proc sort_version` signature `proc sort_version() -> string {` example `sort.sort_version(...)` stability `stable`
- `proc sort_ready` signature `proc sort_ready() -> bool {` example `sort.sort_ready(...)` stability `stable`
- `proc sort_selftest` signature `proc sort_selftest() -> bool {` example `sort.sort_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/statistics.vitl`

Stability: `stable`

- `const STAT_EPSILON` signature `const STAT_EPSILON: f64 = 0.000001` example `statistics.STAT_EPSILON` stability `stable`
- `proc abs_stat` signature `proc abs_stat(value: f64) -> f64 {` example `statistics.abs_stat(...)` stability `stable`
- `proc clamp01` signature `proc clamp01(value: f64) -> f64 {` example `statistics.clamp01(...)` stability `stable`
- `proc has_values_f64` signature `proc has_values_f64(values: [f64]) -> bool {` example `statistics.has_values_f64(...)` stability `stable`
- `proc has_same_length_f64` signature `proc has_same_length_f64(a: [f64], b: [f64]) -> bool {` example `statistics.has_same_length_f64(...)` stability `stable`
- `proc zeros_int` signature `proc zeros_int(count0: int) -> [int] {` example `statistics.zeros_int(...)` stability `stable`
- `proc values_sorted_f64` signature `proc values_sorted_f64(values: [f64]) -> [f64] {` example `statistics.values_sorted_f64(...)` stability `stable`
- `proc values_sorted_int` signature `proc values_sorted_int(values: [int]) -> [int] {` example `statistics.values_sorted_int(...)` stability `stable`
- `proc min2_f64` signature `proc min2_f64(a: f64, b: f64) -> f64 {` example `statistics.min2_f64(...)` stability `stable`
- `proc max2_f64` signature `proc max2_f64(a: f64, b: f64) -> f64 {` example `statistics.max2_f64(...)` stability `stable`
- `proc count` signature `proc count(values: [f64]) -> int {` example `statistics.count(...)` stability `stable`
- `proc sum` signature `proc sum(values: [f64]) -> f64 {` example `statistics.sum(...)` stability `stable`
- `proc product` signature `proc product(values: [f64]) -> f64 {` example `statistics.product(...)` stability `stable`
- `proc sum_of_squares` signature `proc sum_of_squares(values: [f64]) -> f64 {` example `statistics.sum_of_squares(...)` stability `stable`
- `proc mean` signature `proc mean(values: [f64]) -> f64 {` example `statistics.mean(...)` stability `stable`
- `proc geometric_mean` signature `proc geometric_mean(values: [f64]) -> f64 {` example `statistics.geometric_mean(...)` stability `stable`
- `proc harmonic_mean` signature `proc harmonic_mean(values: [f64]) -> f64 {` example `statistics.harmonic_mean(...)` stability `stable`
- `proc root_mean_square` signature `proc root_mean_square(values: [f64]) -> f64 {` example `statistics.root_mean_square(...)` stability `stable`
- `proc min` signature `proc min(values: [f64]) -> f64 {` example `statistics.min(...)` stability `stable`
- `proc max` signature `proc max(values: [f64]) -> f64 {` example `statistics.max(...)` stability `stable`
- `proc range` signature `proc range(values: [f64]) -> f64 {` example `statistics.range(...)` stability `stable`
- `proc midrange` signature `proc midrange(values: [f64]) -> f64 {` example `statistics.midrange(...)` stability `stable`
- `proc mean_absolute_deviation` signature `proc mean_absolute_deviation(values: [f64]) -> f64 {` example `statistics.mean_absolute_deviation(...)` stability `stable`
- `proc variance` signature `proc variance(values: [f64]) -> f64 {` example `statistics.variance(...)` stability `stable`
- `proc population_variance` signature `proc population_variance(values: [f64]) -> f64 {` example `statistics.population_variance(...)` stability `stable`
- `proc sample_variance` signature `proc sample_variance(values: [f64]) -> f64 {` example `statistics.sample_variance(...)` stability `stable`
- `proc stddev` signature `proc stddev(values: [f64]) -> f64 {` example `statistics.stddev(...)` stability `stable`
- `proc population_stddev` signature `proc population_stddev(values: [f64]) -> f64 {` example `statistics.population_stddev(...)` stability `stable`
- `proc sample_stddev` signature `proc sample_stddev(values: [f64]) -> f64 {` example `statistics.sample_stddev(...)` stability `stable`
- `proc median_sorted` signature `proc median_sorted(values: [f64]) -> f64 {` example `statistics.median_sorted(...)` stability `stable`
- `proc median` signature `proc median(values: [f64]) -> f64 {` example `statistics.median(...)` stability `stable`
- `proc quantile` signature `proc quantile(values: [f64], q: f64) -> f64 {` example `statistics.quantile(...)` stability `stable`
- `proc percentile` signature `proc percentile(values: [f64], p: f64) -> f64 {` example `statistics.percentile(...)` stability `stable`
- `proc quartile1` signature `proc quartile1(values: [f64]) -> f64 {` example `statistics.quartile1(...)` stability `stable`
- `proc quartile2` signature `proc quartile2(values: [f64]) -> f64 {` example `statistics.quartile2(...)` stability `stable`
- `proc quartile3` signature `proc quartile3(values: [f64]) -> f64 {` example `statistics.quartile3(...)` stability `stable`
- `proc interquartile_range` signature `proc interquartile_range(values: [f64]) -> f64 {` example `statistics.interquartile_range(...)` stability `stable`
- `proc median_absolute_deviation` signature `proc median_absolute_deviation(values: [f64]) -> f64 {` example `statistics.median_absolute_deviation(...)` stability `stable`
- `proc covariance` signature `proc covariance(a: [f64], b: [f64]) -> f64 {` example `statistics.covariance(...)` stability `stable`
- `proc population_covariance` signature `proc population_covariance(a: [f64], b: [f64]) -> f64 {` example `statistics.population_covariance(...)` stability `stable`
- `proc sample_covariance` signature `proc sample_covariance(a: [f64], b: [f64]) -> f64 {` example `statistics.sample_covariance(...)` stability `stable`
- `proc correlation` signature `proc correlation(a: [f64], b: [f64]) -> f64 {` example `statistics.correlation(...)` stability `stable`
- `proc pearson_correlation` signature `proc pearson_correlation(a: [f64], b: [f64]) -> f64 {` example `statistics.pearson_correlation(...)` stability `stable`
- `proc histogram` signature `proc histogram(values: [f64], bins: int, low: f64, high: f64) -> [int] {` example `statistics.histogram(...)` stability `stable`
- `proc histogram_auto` signature `proc histogram_auto(values: [f64], bins: int) -> [int] {` example `statistics.histogram_auto(...)` stability `stable`
- `proc mode` signature `proc mode(values: [f64]) -> f64 {` example `statistics.mode(...)` stability `stable`
- `proc zscore` signature `proc zscore(value: f64, values: [f64]) -> f64 {` example `statistics.zscore(...)` stability `stable`
- `proc minmax_normalize` signature `proc minmax_normalize(values: [f64]) -> [f64] {` example `statistics.minmax_normalize(...)` stability `stable`
- `proc center` signature `proc center(values: [f64]) -> [f64] {` example `statistics.center(...)` stability `stable`
- `proc trimmed_mean` signature `proc trimmed_mean(values: [f64], trim_count: int) -> f64 {` example `statistics.trimmed_mean(...)` stability `stable`
- `proc winsorized_mean` signature `proc winsorized_mean(values: [f64], trim_count: int) -> f64 {` example `statistics.winsorized_mean(...)` stability `stable`
- `proc coefficient_of_variation` signature `proc coefficient_of_variation(values: [f64]) -> f64 {` example `statistics.coefficient_of_variation(...)` stability `stable`
- `proc slice_f64` signature `proc slice_f64(values: [f64], start: int, end: int) -> [f64] {` example `statistics.slice_f64(...)` stability `stable`
- `proc clamp_bin` signature `proc clamp_bin(index: int, bins: int) -> int {` example `statistics.clamp_bin(...)` stability `stable`
- `proc pow_root` signature `proc pow_root(value: f64, degree: int) -> f64 {` example `statistics.pow_root(...)` stability `stable`
- `proc pow_simple` signature `proc pow_simple(base: f64, exp: int) -> f64 {` example `statistics.pow_simple(...)` stability `stable`
- `proc statistics_version` signature `proc statistics_version() -> string {` example `statistics.statistics_version(...)` stability `stable`
- `proc statistics_ready` signature `proc statistics_ready() -> bool {` example `statistics.statistics_ready(...)` stability `stable`
- `proc statistics_selftest` signature `proc statistics_selftest() -> bool {` example `statistics.statistics_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/topology.vitl`

Stability: `stable`

- `proc contains` signature `proc contains(values: [int], needle: int) -> bool {` example `topology.contains(...)` stability `stable`
- `proc unique` signature `proc unique(values: [int]) -> [int] {` example `topology.unique(...)` stability `stable`
- `proc union_sets` signature `proc union_sets(a: [int], b: [int]) -> [int] {` example `topology.union_sets(...)` stability `stable`
- `proc intersect_sets` signature `proc intersect_sets(a: [int], b: [int]) -> [int] {` example `topology.intersect_sets(...)` stability `stable`
- `proc difference_sets` signature `proc difference_sets(a: [int], b: [int]) -> [int] {` example `topology.difference_sets(...)` stability `stable`
- `proc symmetric_difference` signature `proc symmetric_difference(a: [int], b: [int]) -> [int] {` example `topology.symmetric_difference(...)` stability `stable`
- `proc is_subset` signature `proc is_subset(a: [int], b: [int]) -> bool {` example `topology.is_subset(...)` stability `stable`
- `proc cartesian_product` signature `proc cartesian_product(a: [int], b: [int]) -> [[int]] {` example `topology.cartesian_product(...)` stability `stable`
- `proc power_set` signature `proc power_set(values: [int]) -> [[int]] {` example `topology.power_set(...)` stability `stable`
- `proc adjacency_matrix_to_list` signature `proc adjacency_matrix_to_list(matrix: [[int]]) -> [[int]] {` example `topology.adjacency_matrix_to_list(...)` stability `stable`
- `proc slice_stack` signature `proc slice_stack(values: [int], end: int) -> [int] {` example `topology.slice_stack(...)` stability `stable`
- `proc dfs` signature `proc dfs(graph: [[int]], start: int) -> [int] {` example `topology.dfs(...)` stability `stable`
- `proc topology_version` signature `proc topology_version() -> string {` example `topology.topology_version(...)` stability `stable`
- `proc topology_ready` signature `proc topology_ready() -> bool {` example `topology.topology_ready(...)` stability `stable`
- `proc topology_selftest` signature `proc topology_selftest() -> bool {` example `topology.topology_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/trigonometry.vitl`

Stability: `stable`

- `const PI` signature `const PI: f64 = 3.141592653589793` example `trigonometry.PI` stability `stable`
- `const HALF_PI` signature `const HALF_PI: f64 = 1.5707963267948966` example `trigonometry.HALF_PI` stability `stable`
- `const QUARTER_PI` signature `const QUARTER_PI: f64 = 0.7853981633974483` example `trigonometry.QUARTER_PI` stability `stable`
- `const TWO_PI` signature `const TWO_PI: f64 = 6.283185307179586` example `trigonometry.TWO_PI` stability `stable`
- `const THREE_HALF_PI` signature `const THREE_HALF_PI: f64 = 4.71238898038469` example `trigonometry.THREE_HALF_PI` stability `stable`
- `const DEG_TO_RAD` signature `const DEG_TO_RAD: f64 = 0.017453292519943295` example `trigonometry.DEG_TO_RAD` stability `stable`
- `const RAD_TO_DEG` signature `const RAD_TO_DEG: f64 = 57.29577951308232` example `trigonometry.RAD_TO_DEG` stability `stable`
- `const EPSILON` signature `const EPSILON: f64 = 0.000001` example `trigonometry.EPSILON` stability `stable`
- `const SERIES_STEPS` signature `const SERIES_STEPS: i64 = 8` example `trigonometry.SERIES_STEPS` stability `stable`
- `proc abs_f64` signature `proc abs_f64(value: f64) -> f64 {` example `trigonometry.abs_f64(...)` stability `stable`
- `proc min_f64` signature `proc min_f64(a: f64, b: f64) -> f64 {` example `trigonometry.min_f64(...)` stability `stable`
- `proc max_f64` signature `proc max_f64(a: f64, b: f64) -> f64 {` example `trigonometry.max_f64(...)` stability `stable`
- `proc clamp_f64` signature `proc clamp_f64(value: f64, low: f64, high: f64) -> f64 {` example `trigonometry.clamp_f64(...)` stability `stable`
- `proc sign_f64` signature `proc sign_f64(value: f64) -> f64 {` example `trigonometry.sign_f64(...)` stability `stable`
- `proc is_near_zero` signature `proc is_near_zero(value: f64) -> bool {` example `trigonometry.is_near_zero(...)` stability `stable`
- `proc normalize_degrees` signature `proc normalize_degrees(value: f64) -> f64 {` example `trigonometry.normalize_degrees(...)` stability `stable`
- `proc normalize_radians` signature `proc normalize_radians(value: f64) -> f64 {` example `trigonometry.normalize_radians(...)` stability `stable`
- `proc wrap_radians` signature `proc wrap_radians(value: f64) -> f64 {` example `trigonometry.wrap_radians(...)` stability `stable`
- `proc degrees_to_radians` signature `proc degrees_to_radians(value: f64) -> f64 {` example `trigonometry.degrees_to_radians(...)` stability `stable`
- `proc radians_to_degrees` signature `proc radians_to_degrees(value: f64) -> f64 {` example `trigonometry.radians_to_degrees(...)` stability `stable`
- `proc opposite_angle` signature `proc opposite_angle(value: f64) -> f64 {` example `trigonometry.opposite_angle(...)` stability `stable`
- `proc complement_angle` signature `proc complement_angle(value: f64) -> f64 {` example `trigonometry.complement_angle(...)` stability `stable`
- `proc supplement_angle` signature `proc supplement_angle(value: f64) -> f64 {` example `trigonometry.supplement_angle(...)` stability `stable`
- `proc angle_delta_degrees` signature `proc angle_delta_degrees(from_value: f64, to_value: f64) -> f64 {` example `trigonometry.angle_delta_degrees(...)` stability `stable`
- `proc angle_distance_degrees` signature `proc angle_distance_degrees(a: f64, b: f64) -> f64 {` example `trigonometry.angle_distance_degrees(...)` stability `stable`
- `proc angle_delta_radians` signature `proc angle_delta_radians(from_value: f64, to_value: f64) -> f64 {` example `trigonometry.angle_delta_radians(...)` stability `stable`
- `proc angle_distance_radians` signature `proc angle_distance_radians(a: f64, b: f64) -> f64 {` example `trigonometry.angle_distance_radians(...)` stability `stable`
- `proc quadrant` signature `proc quadrant(value: f64) -> i64 {` example `trigonometry.quadrant(...)` stability `stable`
- `proc is_axis_angle` signature `proc is_axis_angle(value: f64) -> bool {` example `trigonometry.is_axis_angle(...)` stability `stable`
- `proc is_right_angle` signature `proc is_right_angle(value: f64) -> bool {` example `trigonometry.is_right_angle(...)` stability `stable`
- `proc is_straight_angle` signature `proc is_straight_angle(value: f64) -> bool {` example `trigonometry.is_straight_angle(...)` stability `stable`
- `proc is_full_turn_degrees` signature `proc is_full_turn_degrees(value: f64) -> bool {` example `trigonometry.is_full_turn_degrees(...)` stability `stable`
- `proc turns_to_radians` signature `proc turns_to_radians(turns: f64) -> f64 {` example `trigonometry.turns_to_radians(...)` stability `stable`
- `proc radians_to_turns` signature `proc radians_to_turns(value: f64) -> f64 {` example `trigonometry.radians_to_turns(...)` stability `stable`
- `proc square_f64` signature `proc square_f64(value: f64) -> f64 {` example `trigonometry.square_f64(...)` stability `stable`
- `proc cube_f64` signature `proc cube_f64(value: f64) -> f64 {` example `trigonometry.cube_f64(...)` stability `stable`
- `proc reduce_to_half_pi` signature `proc reduce_to_half_pi(value: f64) -> f64 {` example `trigonometry.reduce_to_half_pi(...)` stability `stable`
- `proc factorial_f64` signature `proc factorial_f64(value: i64) -> f64 {` example `trigonometry.factorial_f64(...)` stability `stable`
- `proc sin_series_core` signature `proc sin_series_core(value: f64) -> f64 {` example `trigonometry.sin_series_core(...)` stability `stable`
- `proc cos_series_core` signature `proc cos_series_core(value: f64) -> f64 {` example `trigonometry.cos_series_core(...)` stability `stable`
- `proc sin` signature `proc sin(value: f64) -> f64 {` example `trigonometry.sin(...)` stability `stable`
- `proc cos` signature `proc cos(value: f64) -> f64 {` example `trigonometry.cos(...)` stability `stable`
- `proc tan` signature `proc tan(value: f64) -> f64 {` example `trigonometry.tan(...)` stability `stable`
- `proc sec` signature `proc sec(value: f64) -> f64 {` example `trigonometry.sec(...)` stability `stable`
- `proc csc` signature `proc csc(value: f64) -> f64 {` example `trigonometry.csc(...)` stability `stable`
- `proc cot` signature `proc cot(value: f64) -> f64 {` example `trigonometry.cot(...)` stability `stable`
- `proc sin_degrees` signature `proc sin_degrees(value: f64) -> f64 {` example `trigonometry.sin_degrees(...)` stability `stable`
- `proc cos_degrees` signature `proc cos_degrees(value: f64) -> f64 {` example `trigonometry.cos_degrees(...)` stability `stable`
- `proc tan_degrees` signature `proc tan_degrees(value: f64) -> f64 {` example `trigonometry.tan_degrees(...)` stability `stable`
- `proc sec_degrees` signature `proc sec_degrees(value: f64) -> f64 {` example `trigonometry.sec_degrees(...)` stability `stable`
- `proc csc_degrees` signature `proc csc_degrees(value: f64) -> f64 {` example `trigonometry.csc_degrees(...)` stability `stable`
- `proc cot_degrees` signature `proc cot_degrees(value: f64) -> f64 {` example `trigonometry.cot_degrees(...)` stability `stable`
- `proc atan_series` signature `proc atan_series(value: f64) -> f64 {` example `trigonometry.atan_series(...)` stability `stable`
- `proc atan` signature `proc atan(value: f64) -> f64 {` example `trigonometry.atan(...)` stability `stable`
- `proc sqrt_f64_local` signature `proc sqrt_f64_local(value: f64) -> f64 {` example `trigonometry.sqrt_f64_local(...)` stability `stable`
- `proc asin_series` signature `proc asin_series(value: f64) -> f64 {` example `trigonometry.asin_series(...)` stability `stable`
- `proc asin_atan_identity` signature `proc asin_atan_identity(value: f64) -> f64 {` example `trigonometry.asin_atan_identity(...)` stability `stable`
- `proc asin` signature `proc asin(value: f64) -> f64 {` example `trigonometry.asin(...)` stability `stable`
- `proc acos` signature `proc acos(value: f64) -> f64 {` example `trigonometry.acos(...)` stability `stable`
- `proc atan2` signature `proc atan2(y: f64, x: f64) -> f64 {` example `trigonometry.atan2(...)` stability `stable`
- `proc asec` signature `proc asec(value: f64) -> f64 {` example `trigonometry.asec(...)` stability `stable`
- `proc acsc` signature `proc acsc(value: f64) -> f64 {` example `trigonometry.acsc(...)` stability `stable`
- `proc acot` signature `proc acot(value: f64) -> f64 {` example `trigonometry.acot(...)` stability `stable`
- `proc atan_degrees` signature `proc atan_degrees(value: f64) -> f64 {` example `trigonometry.atan_degrees(...)` stability `stable`
- `proc atan2_degrees` signature `proc atan2_degrees(y: f64, x: f64) -> f64 {` example `trigonometry.atan2_degrees(...)` stability `stable`
- `proc asin_degrees` signature `proc asin_degrees(value: f64) -> f64 {` example `trigonometry.asin_degrees(...)` stability `stable`
- `proc acos_degrees` signature `proc acos_degrees(value: f64) -> f64 {` example `trigonometry.acos_degrees(...)` stability `stable`
- `proc exp_f64_local` signature `proc exp_f64_local(value: f64) -> f64 {` example `trigonometry.exp_f64_local(...)` stability `stable`
- `proc log_f64_local` signature `proc log_f64_local(value: f64) -> f64 {` example `trigonometry.log_f64_local(...)` stability `stable`
- `proc sinh` signature `proc sinh(value: f64) -> f64 {` example `trigonometry.sinh(...)` stability `stable`
- `proc cosh` signature `proc cosh(value: f64) -> f64 {` example `trigonometry.cosh(...)` stability `stable`
- `proc tanh` signature `proc tanh(value: f64) -> f64 {` example `trigonometry.tanh(...)` stability `stable`
- `proc sech` signature `proc sech(value: f64) -> f64 {` example `trigonometry.sech(...)` stability `stable`
- `proc csch` signature `proc csch(value: f64) -> f64 {` example `trigonometry.csch(...)` stability `stable`
- `proc coth` signature `proc coth(value: f64) -> f64 {` example `trigonometry.coth(...)` stability `stable`
- `proc asinh` signature `proc asinh(value: f64) -> f64 {` example `trigonometry.asinh(...)` stability `stable`
- `proc acosh` signature `proc acosh(value: f64) -> f64 {` example `trigonometry.acosh(...)` stability `stable`
- `proc atanh` signature `proc atanh(value: f64) -> f64 {` example `trigonometry.atanh(...)` stability `stable`
- `proc haversin` signature `proc haversin(value: f64) -> f64 {` example `trigonometry.haversin(...)` stability `stable`
- `proc ahaversin` signature `proc ahaversin(value: f64) -> f64 {` example `trigonometry.ahaversin(...)` stability `stable`
- `proc versin` signature `proc versin(value: f64) -> f64 {` example `trigonometry.versin(...)` stability `stable`
- `proc coversin` signature `proc coversin(value: f64) -> f64 {` example `trigonometry.coversin(...)` stability `stable`
- `proc exsec` signature `proc exsec(value: f64) -> f64 {` example `trigonometry.exsec(...)` stability `stable`
- `proc chord` signature `proc chord(value: f64) -> f64 {` example `trigonometry.chord(...)` stability `stable`
- `proc sinc` signature `proc sinc(value: f64) -> f64 {` example `trigonometry.sinc(...)` stability `stable`
- `proc normalized_sinc` signature `proc normalized_sinc(value: f64) -> f64 {` example `trigonometry.normalized_sinc(...)` stability `stable`
- `proc polar_x` signature `proc polar_x(radius: f64, angle_radians: f64) -> f64 {` example `trigonometry.polar_x(...)` stability `stable`
- `proc polar_y` signature `proc polar_y(radius: f64, angle_radians: f64) -> f64 {` example `trigonometry.polar_y(...)` stability `stable`
- `proc polar_x_degrees` signature `proc polar_x_degrees(radius: f64, angle_degrees: f64) -> f64 {` example `trigonometry.polar_x_degrees(...)` stability `stable`
- `proc polar_y_degrees` signature `proc polar_y_degrees(radius: f64, angle_degrees: f64) -> f64 {` example `trigonometry.polar_y_degrees(...)` stability `stable`
- `proc pow_f64_local` signature `proc pow_f64_local(base: f64, exponent: i64) -> f64 {` example `trigonometry.pow_f64_local(...)` stability `stable`
- `proc round_f64_local` signature `proc round_f64_local(value: f64) -> i64 {` example `trigonometry.round_f64_local(...)` stability `stable`
- `proc trigonometry_version` signature `proc trigonometry_version() -> string {` example `trigonometry.trigonometry_version(...)` stability `stable`
- `proc trigonometry_ready` signature `proc trigonometry_ready() -> bool {` example `trigonometry.trigonometry_ready(...)` stability `stable`
- `proc trigonometry_selftest` signature `proc trigonometry_selftest() -> bool {` example `trigonometry.trigonometry_selftest(...)` stability `stable`

## `src/vitte/stdlib/math/vector.vitl`

Stability: `stable`

- `proc vec2` signature `proc vec2(x: int, y: int) -> [int] {` example `vector.vec2(...)` stability `stable`
- `proc vec2_add` signature `proc vec2_add(a: [int], b: [int]) -> [int] {` example `vector.vec2_add(...)` stability `stable`
- `proc vec2_sub` signature `proc vec2_sub(a: [int], b: [int]) -> [int] {` example `vector.vec2_sub(...)` stability `stable`
- `proc vec2_scale` signature `proc vec2_scale(a: [int], factor: int) -> [int] {` example `vector.vec2_scale(...)` stability `stable`
- `proc dot2` signature `proc dot2(a: [int], b: [int]) -> int {` example `vector.dot2(...)` stability `stable`
- `proc norm2_sq` signature `proc norm2_sq(a: [int]) -> int {` example `vector.norm2_sq(...)` stability `stable`
- `proc vector_version` signature `proc vector_version() -> string {` example `vector.vector_version(...)` stability `stable`
- `proc vector_ready` signature `proc vector_ready() -> bool {` example `vector.vector_ready(...)` stability `stable`
- `proc vector_selftest` signature `proc vector_selftest() -> bool {` example `vector.vector_selftest(...)` stability `stable`

## `src/vitte/stdlib/network/http.vitl`

Stability: `stable`

- `form HttpRequest` signature `form HttpRequest {` example `http.HttpRequest` stability `stable`
- `form HttpResponse` signature `form HttpResponse {` example `http.HttpResponse` stability `stable`
- `proc http_request` signature `proc http_request(method: string, path: string, host: string, port: u64) -> HttpRequest {` example `http.http_request(...)` stability `stable`
- `proc http_response` signature `proc http_response(status: u64, body_len: u64) -> HttpResponse {` example `http.http_response(...)` stability `stable`
- `proc http_is_success` signature `proc http_is_success(response: HttpResponse) -> bool {` example `http.http_is_success(...)` stability `stable`
- `proc http_selftest` signature `proc http_selftest() -> bool {` example `http.http_selftest(...)` stability `stable`

## `src/vitte/stdlib/network/socket.vitl`

Stability: `stable`

- `form SocketAddress` signature `form SocketAddress {` example `socket.SocketAddress` stability `stable`
- `form TcpSocket` signature `form TcpSocket {` example `socket.TcpSocket` stability `stable`
- `proc socket_address` signature `proc socket_address(host: string, port: u64) -> SocketAddress {` example `socket.socket_address(...)` stability `stable`
- `proc tcp_socket_new` signature `proc tcp_socket_new(host: string, port: u64) -> TcpSocket {` example `socket.tcp_socket_new(...)` stability `stable`
- `proc tcp_socket_connect` signature `proc tcp_socket_connect(socket: TcpSocket) -> TcpSocket {` example `socket.tcp_socket_connect(...)` stability `stable`
- `proc tcp_socket_set_nonblocking` signature `proc tcp_socket_set_nonblocking(socket: TcpSocket, enabled: bool) -> TcpSocket {` example `socket.tcp_socket_set_nonblocking(...)` stability `stable`
- `proc tcp_socket_ready` signature `proc tcp_socket_ready(socket: TcpSocket) -> bool {` example `socket.tcp_socket_ready(...)` stability `stable`

## `src/vitte/stdlib/network/udp.vitl`

Stability: `stable`

- `form UdpEndpoint` signature `form UdpEndpoint {` example `udp.UdpEndpoint` stability `stable`
- `form UdpPacket` signature `form UdpPacket {` example `udp.UdpPacket` stability `stable`
- `proc udp_bind` signature `proc udp_bind(host: string, port: u64) -> UdpEndpoint {` example `udp.udp_bind(...)` stability `stable`
- `proc udp_packet` signature `proc udp_packet(host: string, port: u64, bytes: u64) -> UdpPacket {` example `udp.udp_packet(...)` stability `stable`
- `proc udp_can_send` signature `proc udp_can_send(endpoint: UdpEndpoint, packet: UdpPacket) -> bool {` example `udp.udp_can_send(...)` stability `stable`

## `src/vitte/stdlib/path/globbing.vitl`

Stability: `stable`

- `form PathGlobbingManifest` signature `form PathGlobbingManifest {` example `globbing.PathGlobbingManifest` stability `stable`
- `form PathGlobbingHealth` signature `form PathGlobbingHealth {` example `globbing.PathGlobbingHealth` stability `stable`
- `form PathGlobbingSummary` signature `form PathGlobbingSummary {` example `globbing.PathGlobbingSummary` stability `stable`
- `proc path_glob` signature `proc path_glob(pattern: string) -> [string] {` example `globbing.path_glob(...)` stability `stable`
- `proc path_glob_recursive` signature `proc path_glob_recursive(pattern: string) -> [string] {` example `globbing.path_glob_recursive(...)` stability `stable`
- `proc path_matches` signature `proc path_matches(p: string, pattern: string) -> int {` example `globbing.path_matches(...)` stability `stable`
- `proc path_is_absolute` signature `proc path_is_absolute(p: string) -> int {` example `globbing.path_is_absolute(...)` stability `stable`
- `proc globbing_version` signature `proc globbing_version() -> string {` example `globbing.globbing_version(...)` stability `stable`
- `proc globbing_ready` signature `proc globbing_ready() -> bool {` example `globbing.globbing_ready(...)` stability `stable`
- `proc globbing_manifest` signature `proc globbing_manifest() -> PathGlobbingManifest {` example `globbing.globbing_manifest(...)` stability `stable`
- `proc globbing_health` signature `proc globbing_health() -> PathGlobbingHealth {` example `globbing.globbing_health(...)` stability `stable`
- `proc globbing_summary` signature `proc globbing_summary() -> PathGlobbingSummary {` example `globbing.globbing_summary(...)` stability `stable`
- `proc path_glob_report` signature `proc path_glob_report(pattern: string) -> [string] {` example `globbing.path_glob_report(...)` stability `stable`
- `proc globbing_selftest` signature `proc globbing_selftest() -> bool {` example `globbing.globbing_selftest(...)` stability `stable`

## `src/vitte/stdlib/path/manipulation.vitl`

Stability: `stable`

- `const PATH_SEPARATOR` signature `const PATH_SEPARATOR: string = "/"` example `manipulation.PATH_SEPARATOR` stability `stable`
- `const PATH_DELIMITER` signature `const PATH_DELIMITER: string = ":"` example `manipulation.PATH_DELIMITER` stability `stable`
- `form Path` signature `form Path {` example `manipulation.Path` stability `stable`
- `form PathManipulationManifest` signature `form PathManipulationManifest {` example `manipulation.PathManipulationManifest` stability `stable`
- `form PathManipulationHealth` signature `form PathManipulationHealth {` example `manipulation.PathManipulationHealth` stability `stable`
- `form PathManipulationSummary` signature `form PathManipulationSummary {` example `manipulation.PathManipulationSummary` stability `stable`
- `proc path_new` signature `proc path_new(p: string) -> Path {` example `manipulation.path_new(...)` stability `stable`
- `proc path_normalize` signature `proc path_normalize(p: string) -> string {` example `manipulation.path_normalize(...)` stability `stable`
- `proc path_resolve` signature `proc path_resolve(p: string) -> string {` example `manipulation.path_resolve(...)` stability `stable`
- `proc path_absolute` signature `proc path_absolute(p: string) -> string {` example `manipulation.path_absolute(...)` stability `stable`
- `proc path_relative` signature `proc path_relative(base: string, target: string) -> string {` example `manipulation.path_relative(...)` stability `stable`
- `proc path_get_parent` signature `proc path_get_parent(p: string) -> string {` example `manipulation.path_get_parent(...)` stability `stable`
- `proc path_get_filename` signature `proc path_get_filename(p: string) -> string {` example `manipulation.path_get_filename(...)` stability `stable`
- `proc path_get_basename` signature `proc path_get_basename(p: string) -> string {` example `manipulation.path_get_basename(...)` stability `stable`
- `proc path_get_stem` signature `proc path_get_stem(p: string) -> string {` example `manipulation.path_get_stem(...)` stability `stable`
- `proc path_get_extension` signature `proc path_get_extension(p: string) -> string {` example `manipulation.path_get_extension(...)` stability `stable`
- `proc path_get_directory` signature `proc path_get_directory(p: string) -> string {` example `manipulation.path_get_directory(...)` stability `stable`
- `proc path_get_root` signature `proc path_get_root(p: string) -> string {` example `manipulation.path_get_root(...)` stability `stable`
- `proc path_join` signature `proc path_join(base: string, component: string) -> string {` example `manipulation.path_join(...)` stability `stable`
- `proc path_join_multi` signature `proc path_join_multi(base: string, components: [string]) -> string {` example `manipulation.path_join_multi(...)` stability `stable`
- `proc path_with_extension` signature `proc path_with_extension(p: string, ext: string) -> string {` example `manipulation.path_with_extension(...)` stability `stable`
- `proc path_with_filename` signature `proc path_with_filename(p: string, filename: string) -> string {` example `manipulation.path_with_filename(...)` stability `stable`
- `proc path_exists` signature `proc path_exists(p: string) -> int {` example `manipulation.path_exists(...)` stability `stable`
- `proc path_is_file` signature `proc path_is_file(p: string) -> int {` example `manipulation.path_is_file(...)` stability `stable`
- `proc path_is_dir` signature `proc path_is_dir(p: string) -> int {` example `manipulation.path_is_dir(...)` stability `stable`
- `proc path_is_symlink` signature `proc path_is_symlink(p: string) -> int {` example `manipulation.path_is_symlink(...)` stability `stable`
- `proc path_is_absolute` signature `proc path_is_absolute(p: string) -> int {` example `manipulation.path_is_absolute(...)` stability `stable`
- `proc path_is_relative` signature `proc path_is_relative(p: string) -> int {` example `manipulation.path_is_relative(...)` stability `stable`
- `proc path_is_hidden` signature `proc path_is_hidden(p: string) -> int {` example `manipulation.path_is_hidden(...)` stability `stable`
- `proc path_equals` signature `proc path_equals(p1: string, p2: string) -> int {` example `manipulation.path_equals(...)` stability `stable`
- `proc path_starts_with` signature `proc path_starts_with(p: string, prefix: string) -> int {` example `manipulation.path_starts_with(...)` stability `stable`
- `proc path_ends_with` signature `proc path_ends_with(p: string, suffix: string) -> int {` example `manipulation.path_ends_with(...)` stability `stable`
- `proc path_clean` signature `proc path_clean(p: string) -> string {` example `manipulation.path_clean(...)` stability `stable`
- `proc path_canonicalize` signature `proc path_canonicalize(p: string) -> string {` example `manipulation.path_canonicalize(...)` stability `stable`
- `proc path_simplify` signature `proc path_simplify(p: string) -> string {` example `manipulation.path_simplify(...)` stability `stable`
- `proc path_expand_home` signature `proc path_expand_home(p: string) -> string {` example `manipulation.path_expand_home(...)` stability `stable`
- `proc path_expand_env` signature `proc path_expand_env(p: string) -> string {` example `manipulation.path_expand_env(...)` stability `stable`
- `proc path_version` signature `proc path_version() -> string {` example `manipulation.path_version(...)` stability `stable`
- `proc path_ready` signature `proc path_ready() -> bool {` example `manipulation.path_ready(...)` stability `stable`
- `proc manipulation_version` signature `proc manipulation_version() -> string {` example `manipulation.manipulation_version(...)` stability `stable`
- `proc manipulation_ready` signature `proc manipulation_ready() -> bool {` example `manipulation.manipulation_ready(...)` stability `stable`
- `proc manipulation_manifest` signature `proc manipulation_manifest() -> PathManipulationManifest {` example `manipulation.manipulation_manifest(...)` stability `stable`
- `proc manipulation_health` signature `proc manipulation_health() -> PathManipulationHealth {` example `manipulation.manipulation_health(...)` stability `stable`
- `proc manipulation_summary` signature `proc manipulation_summary() -> PathManipulationSummary {` example `manipulation.manipulation_summary(...)` stability `stable`
- `proc path_report` signature `proc path_report(p: string) -> Path {` example `manipulation.path_report(...)` stability `stable`
- `proc path_selftest` signature `proc path_selftest() -> bool {` example `manipulation.path_selftest(...)` stability `stable`

## `src/vitte/stdlib/path/special.vitl`

Stability: `stable`

- `form PathSpecialManifest` signature `form PathSpecialManifest {` example `special.PathSpecialManifest` stability `stable`
- `form PathSpecialHealth` signature `form PathSpecialHealth {` example `special.PathSpecialHealth` stability `stable`
- `form PathSpecialSummary` signature `form PathSpecialSummary {` example `special.PathSpecialSummary` stability `stable`
- `proc home_dir` signature `proc home_dir() -> string {` example `special.home_dir(...)` stability `stable`
- `proc temp_dir` signature `proc temp_dir() -> string {` example `special.temp_dir(...)` stability `stable`
- `proc current_dir` signature `proc current_dir() -> string {` example `special.current_dir(...)` stability `stable`
- `proc config_dir` signature `proc config_dir() -> string {` example `special.config_dir(...)` stability `stable`
- `proc cache_dir` signature `proc cache_dir() -> string {` example `special.cache_dir(...)` stability `stable`
- `proc desktop_dir` signature `proc desktop_dir() -> string {` example `special.desktop_dir(...)` stability `stable`
- `proc documents_dir` signature `proc documents_dir() -> string {` example `special.documents_dir(...)` stability `stable`
- `proc downloads_dir` signature `proc downloads_dir() -> string {` example `special.downloads_dir(...)` stability `stable`
- `proc get_path_env` signature `proc get_path_env() -> [string] {` example `special.get_path_env(...)` stability `stable`
- `proc find_in_path` signature `proc find_in_path(executable: string) -> string {` example `special.find_in_path(...)` stability `stable`
- `proc add_to_path` signature `proc add_to_path(directory: string) -> int {` example `special.add_to_path(...)` stability `stable`
- `proc remove_from_path` signature `proc remove_from_path(directory: string) -> int {` example `special.remove_from_path(...)` stability `stable`
- `proc special_version` signature `proc special_version() -> string {` example `special.special_version(...)` stability `stable`
- `proc special_ready` signature `proc special_ready() -> bool {` example `special.special_ready(...)` stability `stable`
- `proc special_manifest` signature `proc special_manifest() -> PathSpecialManifest {` example `special.special_manifest(...)` stability `stable`
- `proc special_health` signature `proc special_health() -> PathSpecialHealth {` example `special.special_health(...)` stability `stable`
- `proc special_summary` signature `proc special_summary() -> PathSpecialSummary {` example `special.special_summary(...)` stability `stable`
- `proc special_report` signature `proc special_report() -> [string] {` example `special.special_report(...)` stability `stable`
- `proc special_selftest` signature `proc special_selftest() -> bool {` example `special.special_selftest(...)` stability `stable`

## `src/vitte/stdlib/path/walker.vitl`

Stability: `stable`

- `form PathWalker` signature `form PathWalker {` example `walker.PathWalker` stability `stable`
- `form PathWalkerManifest` signature `form PathWalkerManifest {` example `walker.PathWalkerManifest` stability `stable`
- `form PathWalkerHealth` signature `form PathWalkerHealth {` example `walker.PathWalkerHealth` stability `stable`
- `form PathWalkerSummary` signature `form PathWalkerSummary {` example `walker.PathWalkerSummary` stability `stable`
- `proc path_walk` signature `proc path_walk(root: string) -> PathWalker {` example `walker.path_walk(...)` stability `stable`
- `proc path_walk_recursive` signature `proc path_walk_recursive(root: string) -> PathWalker {` example `walker.path_walk_recursive(...)` stability `stable`
- `proc path_walker_next` signature `proc path_walker_next(w: PathWalker) -> string {` example `walker.path_walker_next(...)` stability `stable`
- `proc path_walker_has_next` signature `proc path_walker_has_next(w: PathWalker) -> int {` example `walker.path_walker_has_next(...)` stability `stable`
- `proc walker_version` signature `proc walker_version() -> string {` example `walker.walker_version(...)` stability `stable`
- `proc walker_ready` signature `proc walker_ready() -> bool {` example `walker.walker_ready(...)` stability `stable`
- `proc walker_manifest` signature `proc walker_manifest() -> PathWalkerManifest {` example `walker.walker_manifest(...)` stability `stable`
- `proc walker_health` signature `proc walker_health() -> PathWalkerHealth {` example `walker.walker_health(...)` stability `stable`
- `proc walker_summary` signature `proc walker_summary() -> PathWalkerSummary {` example `walker.walker_summary(...)` stability `stable`
- `proc walker_report` signature `proc walker_report(root: string) -> PathWalker {` example `walker.walker_report(...)` stability `stable`
- `proc walker_selftest` signature `proc walker_selftest() -> bool {` example `walker.walker_selftest(...)` stability `stable`

## `src/vitte/stdlib/platform/abi.vitl`

Stability: `stable`

- `form PlatformInfo` signature `form PlatformInfo {` example `abi.PlatformInfo` stability `stable`
- `form PlatformFeature` signature `form PlatformFeature {` example `abi.PlatformFeature` stability `stable`
- `proc platform_info` signature `proc platform_info() -> PlatformInfo { give compiler_platform_info(); }` example `abi.platform_info(...)` stability `stable`
- `proc platform_feature` signature `proc platform_feature(name: string) -> PlatformFeature { give compiler_platform_feature(name); }` example `abi.platform_feature(...)` stability `stable`
- `proc supports_threads` signature `proc supports_threads() -> bool { give platform_feature("threads").enabled; }` example `abi.supports_threads(...)` stability `stable`
- `proc supports_filesystem` signature `proc supports_filesystem() -> bool { give platform_feature("filesystem").enabled; }` example `abi.supports_filesystem(...)` stability `stable`
- `proc supports_process` signature `proc supports_process() -> bool { give platform_feature("process").enabled; }` example `abi.supports_process(...)` stability `stable`
- `proc supports_network` signature `proc supports_network() -> bool { give platform_feature("network").enabled; }` example `abi.supports_network(...)` stability `stable`

## `src/vitte/stdlib/platform/embedded.vitl`

Stability: `stable`

- `form EmbeddedPin` signature `form EmbeddedPin { number: u32 }` example `embedded.EmbeddedPin` stability `stable`
- `form EmbeddedClock` signature `form EmbeddedClock { hz: u64 }` example `embedded.EmbeddedClock` stability `stable`
- `form EmbeddedError` signature `form EmbeddedError { message: String }` example `embedded.EmbeddedError` stability `stable`
- `proc embedded_available` signature `proc embedded_available() -> bool { give compiler_platform_family() == "embedded"; }` example `embedded.embedded_available(...)` stability `stable`
- `proc monotonic_ticks` signature `proc monotonic_ticks() -> u64 { give compiler_embedded_monotonic_ticks(); }` example `embedded.monotonic_ticks(...)` stability `stable`
- `proc clock_frequency` signature `proc clock_frequency() -> EmbeddedClock { give compiler_embedded_clock_frequency(); }` example `embedded.clock_frequency(...)` stability `stable`
- `proc pin` signature `proc pin(number: u32) -> EmbeddedPin { give EmbeddedPin { number: number }; }` example `embedded.pin(...)` stability `stable`
- `proc read_pin` signature `proc read_pin(pin_value: EmbeddedPin) -> Result<bool, EmbeddedError> { give compiler_embedded_read_pin(pin_value); }` example `embedded.read_pin(...)` stability `stable`
- `proc write_pin` signature `proc write_pin(pin_value: EmbeddedPin, value: bool) -> Result<(), EmbeddedError> { give compiler_embedded_write_pin(pin_value, value); }` example `embedded.write_pin(...)` stability `stable`

## `src/vitte/stdlib/platform/posix.vitl`

Stability: `stable`

- `form PosixFd` signature `form PosixFd { raw: int }` example `posix.PosixFd` stability `stable`
- `form PosixError` signature `form PosixError { errno: int message: String }` example `posix.PosixError` stability `stable`
- `form PosixStat` signature `form PosixStat { mode: u32 size: u64 mtime: i64 }` example `posix.PosixStat` stability `stable`
- `proc posix_available` signature `proc posix_available() -> bool { give compiler_platform_family() == "posix"; }` example `posix.posix_available(...)` stability `stable`
- `proc posix_open` signature `proc posix_open(path: PathBuf, flags: int, mode: u32) -> Result<PosixFd, PosixError> { give compiler_posix_open(path, flags, mode); }` example `posix.posix_open(...)` stability `stable`
- `proc posix_close` signature `proc posix_close(fd: PosixFd) -> Result<(), PosixError> { give compiler_posix_close(fd); }` example `posix.posix_close(...)` stability `stable`
- `proc posix_read` signature `proc posix_read(fd: PosixFd, buffer: ref mut [byte]) -> Result<usize, PosixError> { give compiler_posix_read(fd, buffer); }` example `posix.posix_read(...)` stability `stable`
- `proc posix_write` signature `proc posix_write(fd: PosixFd, buffer: [byte]) -> Result<usize, PosixError> { give compiler_posix_write(fd, buffer); }` example `posix.posix_write(...)` stability `stable`
- `proc posix_stat` signature `proc posix_stat(path: PathBuf) -> Result<PosixStat, PosixError> { give compiler_posix_stat(path); }` example `posix.posix_stat(...)` stability `stable`

## `src/vitte/stdlib/platform/wasm.vitl`

Stability: `stable`

- `form WasmMemory` signature `form WasmMemory { pages: u32 }` example `wasm.WasmMemory` stability `stable`
- `form WasmImport` signature `form WasmImport { module: String name: String }` example `wasm.WasmImport` stability `stable`
- `proc wasm_available` signature `proc wasm_available() -> bool { give compiler_platform_family() == "wasm"; }` example `wasm.wasm_available(...)` stability `stable`
- `proc wasm_memory_pages` signature `proc wasm_memory_pages() -> u32 { give compiler_wasm_memory_pages(); }` example `wasm.wasm_memory_pages(...)` stability `stable`
- `proc wasm_grow_memory` signature `proc wasm_grow_memory(pages: u32) -> Result<WasmMemory, string> { give compiler_wasm_grow_memory(pages); }` example `wasm.wasm_grow_memory(...)` stability `stable`
- `proc wasm_import` signature `proc wasm_import(module: String, name: String) -> WasmImport { give WasmImport { module: module, name: name }; }` example `wasm.wasm_import(...)` stability `stable`
- `proc wasm_call` signature `proc wasm_call(import_value: WasmImport, args: Vec<u64>) -> Result<u64, string> { give compiler_wasm_call(import_value, args); }` example `wasm.wasm_call(...)` stability `stable`

## `src/vitte/stdlib/platform/windows.vitl`

Stability: `stable`

- `form WindowsHandle` signature `form WindowsHandle { raw: u64 }` example `windows.WindowsHandle` stability `stable`
- `form WindowsError` signature `form WindowsError { code: u32 message: String }` example `windows.WindowsError` stability `stable`
- `form WindowsPath` signature `form WindowsPath { value: String verbatim: bool }` example `windows.WindowsPath` stability `stable`
- `proc windows_available` signature `proc windows_available() -> bool { give compiler_platform_family() == "windows"; }` example `windows.windows_available(...)` stability `stable`
- `proc windows_path` signature `proc windows_path(path: PathBuf) -> WindowsPath { give compiler_windows_path(path); }` example `windows.windows_path(...)` stability `stable`
- `proc windows_open` signature `proc windows_open(path: WindowsPath, access: u32) -> Result<WindowsHandle, WindowsError> { give compiler_windows_open(path, access); }` example `windows.windows_open(...)` stability `stable`
- `proc windows_close` signature `proc windows_close(handle: WindowsHandle) -> Result<(), WindowsError> { give compiler_windows_close(handle); }` example `windows.windows_close(...)` stability `stable`
- `proc windows_read` signature `proc windows_read(handle: WindowsHandle, buffer: ref mut [byte]) -> Result<usize, WindowsError> { give compiler_windows_read(handle, buffer); }` example `windows.windows_read(...)` stability `stable`
- `proc windows_write` signature `proc windows_write(handle: WindowsHandle, buffer: [byte]) -> Result<usize, WindowsError> { give compiler_windows_write(handle, buffer); }` example `windows.windows_write(...)` stability `stable`

## `src/vitte/stdlib/profiling/profiler.vitl`

Stability: `stable`

- `form ProfilerSample` signature `form ProfilerSample {` example `profiler.ProfilerSample` stability `stable`
- `form ProfilerReport` signature `form ProfilerReport {` example `profiler.ProfilerReport` stability `stable`
- `proc profile_report` signature `proc profile_report(target: string, samples: u64, total_ns: u64, allocations: u64, hottest: string) -> ProfilerReport {` example `profiler.profile_report(...)` stability `stable`
- `proc profiler_summary` signature `proc profiler_summary(report: ProfilerReport) -> string {` example `profiler.profiler_summary(...)` stability `stable`
- `proc profiler_empty` signature `proc profiler_empty() -> ProfilerReport {` example `profiler.profiler_empty(...)` stability `stable`
- `proc profiler_selftest` signature `proc profiler_selftest() -> bool {` example `profiler.profiler_selftest(...)` stability `stable`

## `src/vitte/stdlib/std/atomic.vitl`

Stability: `stable`

- `pick Ordering` signature `pick Ordering { Relaxed Acquire Release AcqRel SeqCst }` example `atomic.Ordering` stability `stable`
- `form AtomicBool` signature `form AtomicBool { value: bool }` example `atomic.AtomicBool` stability `stable`
- `form AtomicUsize` signature `form AtomicUsize { value: usize }` example `atomic.AtomicUsize` stability `stable`
- `form AtomicU64` signature `form AtomicU64 { value: u64 }` example `atomic.AtomicU64` stability `stable`
- `form AtomicI64` signature `form AtomicI64 { value: i64 }` example `atomic.AtomicI64` stability `stable`
- `proc atomic_bool` signature `proc atomic_bool(value: bool) -> AtomicBool { give AtomicBool { value: value }; }` example `atomic.atomic_bool(...)` stability `stable`
- `proc atomic_usize` signature `proc atomic_usize(value: usize) -> AtomicUsize { give AtomicUsize { value: value }; }` example `atomic.atomic_usize(...)` stability `stable`
- `proc atomic_u64` signature `proc atomic_u64(value: u64) -> AtomicU64 { give AtomicU64 { value: value }; }` example `atomic.atomic_u64(...)` stability `stable`
- `proc atomic_i64` signature `proc atomic_i64(value: i64) -> AtomicI64 { give AtomicI64 { value: value }; }` example `atomic.atomic_i64(...)` stability `stable`
- `proc load_bool` signature `proc load_bool(value: ref AtomicBool, ordering: Ordering) -> bool { give compiler_atomic_load_bool(value, ordering); }` example `atomic.load_bool(...)` stability `stable`
- `proc store_bool` signature `proc store_bool(value: ref AtomicBool, next: bool, ordering: Ordering) { compiler_atomic_store_bool(value, next, ordering); }` example `atomic.store_bool(...)` stability `stable`
- `proc swap_bool` signature `proc swap_bool(value: ref AtomicBool, next: bool, ordering: Ordering) -> bool { give compiler_atomic_swap_bool(value, next, ordering); }` example `atomic.swap_bool(...)` stability `stable`
- `proc compare_exchange_bool` signature `proc compare_exchange_bool(value: ref AtomicBool, current: bool, next: bool, success: Ordering, failure: Ordering) -> Result<bool, bool> {` example `atomic.compare_exchange_bool(...)` stability `stable`
- `proc load_usize` signature `proc load_usize(value: ref AtomicUsize, ordering: Ordering) -> usize { give compiler_atomic_load_usize(value, ordering); }` example `atomic.load_usize(...)` stability `stable`
- `proc store_usize` signature `proc store_usize(value: ref AtomicUsize, next: usize, ordering: Ordering) { compiler_atomic_store_usize(value, next, ordering); }` example `atomic.store_usize(...)` stability `stable`
- `proc fetch_add_usize` signature `proc fetch_add_usize(value: ref AtomicUsize, amount: usize, ordering: Ordering) -> usize { give compiler_atomic_fetch_add_usize(value, amount, ordering); }` example `atomic.fetch_add_usize(...)` stability `stable`
- `proc fetch_sub_usize` signature `proc fetch_sub_usize(value: ref AtomicUsize, amount: usize, ordering: Ordering) -> usize { give compiler_atomic_fetch_sub_usize(value, amount, ordering); }` example `atomic.fetch_sub_usize(...)` stability `stable`
- `proc swap_usize` signature `proc swap_usize(value: ref AtomicUsize, next: usize, ordering: Ordering) -> usize { give compiler_atomic_swap_usize(value, next, ordering); }` example `atomic.swap_usize(...)` stability `stable`
- `proc compare_exchange_usize` signature `proc compare_exchange_usize(value: ref AtomicUsize, current: usize, next: usize, success: Ordering, failure: Ordering) -> Result<usize, usize> {` example `atomic.compare_exchange_usize(...)` stability `stable`
- `proc fetch_or_usize` signature `proc fetch_or_usize(value: ref AtomicUsize, mask: usize, ordering: Ordering) -> usize { give compiler_atomic_fetch_or_usize(value, mask, ordering); }` example `atomic.fetch_or_usize(...)` stability `stable`
- `proc fetch_and_usize` signature `proc fetch_and_usize(value: ref AtomicUsize, mask: usize, ordering: Ordering) -> usize { give compiler_atomic_fetch_and_usize(value, mask, ordering); }` example `atomic.fetch_and_usize(...)` stability `stable`
- `proc fetch_xor_usize` signature `proc fetch_xor_usize(value: ref AtomicUsize, mask: usize, ordering: Ordering) -> usize { give compiler_atomic_fetch_xor_usize(value, mask, ordering); }` example `atomic.fetch_xor_usize(...)` stability `stable`

## `src/vitte/stdlib/std/base64.vitl`

Stability: `stable`

- `pick Base64Alphabet` signature `pick Base64Alphabet {` example `base64.Base64Alphabet` stability `stable`
- `form Base64Config` signature `form Base64Config {` example `base64.Base64Config` stability `stable`
- `form Base64Error` signature `form Base64Error {` example `base64.Base64Error` stability `stable`
- `proc base64_standard` signature `proc base64_standard() -> Base64Config { give Base64Config { alphabet: Base64Alphabet.Standard, padding: true }; }` example `base64.base64_standard(...)` stability `stable`
- `proc base64_url_safe` signature `proc base64_url_safe() -> Base64Config { give Base64Config { alphabet: Base64Alphabet.UrlSafe, padding: false }; }` example `base64.base64_url_safe(...)` stability `stable`
- `proc base64_encode` signature `proc base64_encode(bytes: [byte], config: Base64Config) -> String {` example `base64.base64_encode(...)` stability `stable`
- `proc base64_decode` signature `proc base64_decode(text: Utf8View, config: Base64Config) -> Result<Vec<byte>, Base64Error> {` example `base64.base64_decode(...)` stability `stable`

## `src/vitte/stdlib/std/bench.vitl`

Stability: `stable`

- `form Benchmark` signature `form Benchmark { name: String iterations: u64 }` example `bench.Benchmark` stability `stable`
- `form BenchReport` signature `form BenchReport { name: String iterations: u64 nanos: u128 passed_threshold: bool }` example `bench.BenchReport` stability `stable`
- `form BenchSuite` signature `form BenchSuite { benches: Vec<Benchmark> }` example `bench.BenchSuite` stability `stable`
- `form BenchSuiteReport` signature `form BenchSuiteReport { reports: Vec<BenchReport> total_nanos: u128 }` example `bench.BenchSuiteReport` stability `stable`
- `proc benchmark` signature `proc benchmark(name: String, iterations: u64) -> Benchmark { give Benchmark { name: name, iterations: iterations }; }` example `bench.benchmark(...)` stability `stable`
- `proc run` signature `proc run(bench: Benchmark, body: proc()) -> BenchReport {` example `bench.run(...)` stability `stable`
- `proc threshold` signature `proc threshold(report: BenchReport, max_nanos: u128) -> bool { give report.nanos <= max_nanos; }` example `bench.threshold(...)` stability `stable`
- `proc report_text` signature `proc report_text(report: BenchReport) -> String {` example `bench.report_text(...)` stability `stable`
- `proc run_suite` signature `proc run_suite(suite: BenchSuite, body: proc(Benchmark)) -> BenchSuiteReport {` example `bench.run_suite(...)` stability `stable`

## `src/vitte/stdlib/std/calendar.vitl`

Stability: `stable`

- `pick Weekday` signature `pick Weekday { Monday Tuesday Wednesday Thursday Friday Saturday Sunday }` example `calendar.Weekday` stability `stable`
- `form Date` signature `form Date { year: i32 month: u8 day: u8 }` example `calendar.Date` stability `stable`
- `form CalendarError` signature `form CalendarError { message: String }` example `calendar.CalendarError` stability `stable`
- `proc date` signature `proc date(year: i32, month: u8, day: u8) -> Result<Date, CalendarError> {` example `calendar.date(...)` stability `stable`
- `proc is_leap_year` signature `proc is_leap_year(year: i32) -> bool {` example `calendar.is_leap_year(...)` stability `stable`
- `proc weekday` signature `proc weekday(value: Date) -> Weekday { give compiler_calendar_weekday(value); }` example `calendar.weekday(...)` stability `stable`
- `proc days_in_month` signature `proc days_in_month(year: i32, month: u8) -> u8 {` example `calendar.days_in_month(...)` stability `stable`
- `proc date_format` signature `proc date_format(value: Date, pattern: string) -> String { give compiler_calendar_format(value, pattern); }` example `calendar.date_format(...)` stability `stable`

## `src/vitte/stdlib/std/cli.vitl`

Stability: `stable`

- `form Flag` signature `form Flag { name: String short: Option<rune> help: String takes_value: bool }` example `cli.Flag` stability `stable`
- `form CliApp` signature `form CliApp { name: String about: String flags: Vec<Flag> }` example `cli.CliApp` stability `stable`
- `form CliMatches` signature `form CliMatches { values: HashMap<String, String> present: HashSet<String> }` example `cli.CliMatches` stability `stable`
- `form CliError` signature `form CliError { message: String flag: Option<String> }` example `cli.CliError` stability `stable`
- `proc cli_app` signature `proc cli_app(name: String, about: String) -> CliApp { give CliApp { name: name, about: about, flags: vec_new<Flag>() }; }` example `cli.cli_app(...)` stability `stable`
- `proc flag` signature `proc flag(name: String, short: Option<rune>, help: String, takes_value: bool) -> Flag { give Flag { name: name, short: short, help: help, takes_value: takes_value }; }` example `cli.flag(...)` stability `stable`
- `proc add_flag` signature `proc add_flag(app: ref mut CliApp, value: Flag) { vec_push<Flag>(&mut ((*app).flags), value); }` example `cli.add_flag(...)` stability `stable`
- `proc parse` signature `proc parse(app: CliApp, args: Iterator<String>) -> Result<CliMatches, CliError> { give compiler_cli_parse(app, args); }` example `cli.parse(...)` stability `stable`
- `proc help` signature `proc help(app: CliApp) -> String { give compiler_cli_help(app); }` example `cli.help(...)` stability `stable`

## `src/vitte/stdlib/std/csv.vitl`

Stability: `stable`

- `form CsvOptions` signature `form CsvOptions {` example `csv.CsvOptions` stability `stable`
- `form CsvRecord` signature `form CsvRecord {` example `csv.CsvRecord` stability `stable`
- `form CsvError` signature `form CsvError {` example `csv.CsvError` stability `stable`
- `proc csv_default_options` signature `proc csv_default_options() -> CsvOptions { give CsvOptions { delimiter: 44, has_headers: false }; }` example `csv.csv_default_options(...)` stability `stable`
- `proc csv_parse` signature `proc csv_parse(text: Utf8View, options: CsvOptions) -> Result<Vec<CsvRecord>, CsvError> {` example `csv.csv_parse(...)` stability `stable`
- `proc csv_write` signature `proc csv_write(records: Vec<CsvRecord>, options: CsvOptions) -> Result<String, CsvError> {` example `csv.csv_write(...)` stability `stable`
- `proc csv_record` signature `proc csv_record(fields: Vec<String>) -> CsvRecord { give CsvRecord { fields: fields }; }` example `csv.csv_record(...)` stability `stable`

## `src/vitte/stdlib/std/diff.vitl`

Stability: `stable`

- `pick DiffTag` signature `pick DiffTag { Equal Insert Delete Replace }` example `diff.DiffTag` stability `stable`
- `form DiffHunk` signature `form DiffHunk { tag: DiffTag old_text: String new_text: String }` example `diff.DiffHunk` stability `stable`
- `form DiffOptions` signature `form DiffOptions { context: usize }` example `diff.DiffOptions` stability `stable`
- `proc diff_options` signature `proc diff_options() -> DiffOptions { give DiffOptions { context: 3 }; }` example `diff.diff_options(...)` stability `stable`
- `proc diff_lines` signature `proc diff_lines(old_text: Utf8View, new_text: Utf8View, options: DiffOptions) -> Vec<DiffHunk> { give compiler_diff_lines(old_text, new_text, options); }` example `diff.diff_lines(...)` stability `stable`
- `proc diff_words` signature `proc diff_words(old_text: Utf8View, new_text: Utf8View, options: DiffOptions) -> Vec<DiffHunk> { give compiler_diff_words(old_text, new_text, options); }` example `diff.diff_words(...)` stability `stable`
- `proc diff_unified` signature `proc diff_unified(old_text: Utf8View, new_text: Utf8View, options: DiffOptions) -> String { give compiler_diff_unified(old_text, new_text, options); }` example `diff.diff_unified(...)` stability `stable`

## `src/vitte/stdlib/std/env.vitl`

Stability: `stable`

- `form EnvError` signature `form EnvError { name: string message: string }` example `env.EnvError` stability `stable`
- `form EnvVar` signature `form EnvVar { name: String value: String unicode: bool os_specific: bool }` example `env.EnvVar` stability `stable`
- `proc args` signature `proc args() -> Iterator<String> { give compiler_backend_env_args(); }` example `env.args(...)` stability `stable`
- `proc var` signature `proc var(name: string) -> Result<String, EnvError> { give compiler_backend_env_var(name); }` example `env.var(...)` stability `stable`
- `proc vars` signature `proc vars() -> Iterator<EnvVar> { give compiler_backend_env_vars(); }` example `env.vars(...)` stability `stable`
- `proc set_var` signature `proc set_var(name: string, value: string) -> Result<(), EnvError> { give compiler_backend_env_set_var(name, value); }` example `env.set_var(...)` stability `stable`
- `proc set_var_unicode` signature `proc set_var_unicode(name: String, value: String) -> Result<(), EnvError> { give compiler_backend_env_set_var_unicode(name, value); }` example `env.set_var_unicode(...)` stability `stable`
- `proc remove_var` signature `proc remove_var(name: string) -> Result<(), EnvError> { give compiler_backend_env_remove_var(name); }` example `env.remove_var(...)` stability `stable`
- `proc current_dir` signature `proc current_dir() -> Result<Path, EnvError> { give compiler_backend_env_current_dir(); }` example `env.current_dir(...)` stability `stable`
- `proc set_current_dir` signature `proc set_current_dir(path_value: Path) -> Result<(), EnvError> { give compiler_backend_env_set_current_dir(path_value); }` example `env.set_current_dir(...)` stability `stable`
- `proc home_dir` signature `proc home_dir() -> Option<Path> { give compiler_backend_env_home_dir(); }` example `env.home_dir(...)` stability `stable`
- `proc temp_dir` signature `proc temp_dir() -> Path { give compiler_backend_env_temp_dir(); }` example `env.temp_dir(...)` stability `stable`
- `proc os_string_var` signature `proc os_string_var(name: String) -> Result<String, EnvError> { give compiler_backend_env_os_string_var(name); }` example `env.os_string_var(...)` stability `stable`
- `proc args_os` signature `proc args_os() -> Iterator<String> { give compiler_backend_env_args_os(); }` example `env.args_os(...)` stability `stable`
- `proc var_unicode` signature `proc var_unicode(name: String) -> Result<String, EnvError> { give compiler_backend_env_var_unicode(name); }` example `env.var_unicode(...)` stability `stable`
- `proc config_dir` signature `proc config_dir() -> Option<Path> { give compiler_backend_env_config_dir(); }` example `env.config_dir(...)` stability `stable`
- `proc data_dir` signature `proc data_dir() -> Option<Path> { give compiler_backend_env_data_dir(); }` example `env.data_dir(...)` stability `stable`
- `proc cache_dir` signature `proc cache_dir() -> Option<Path> { give compiler_backend_env_cache_dir(); }` example `env.cache_dir(...)` stability `stable`

## `src/vitte/stdlib/std/error.vitl`

Stability: `stable`

- `form Backtrace` signature `form Backtrace {` example `error.Backtrace` stability `stable`
- `form Error` signature `form Error {` example `error.Error` stability `stable`
- `form ErrorTrait` signature `form ErrorTrait<T> {` example `error.ErrorTrait` stability `stable`
- `proc error_new` signature `proc error_new(message: String) -> Error {` example `error.error_new(...)` stability `stable`
- `proc error_with_source` signature `proc error_with_source(message: String, source: Error) -> Error {` example `error.error_with_source(...)` stability `stable`
- `proc capture_backtrace` signature `proc capture_backtrace() -> Backtrace { give compiler_error_capture_backtrace(); }` example `error.capture_backtrace(...)` stability `stable`
- `proc error_with_backtrace` signature `proc error_with_backtrace(error: Error) -> Error { give compiler_error_attach_backtrace(error, capture_backtrace()); }` example `error.error_with_backtrace(...)` stability `stable`
- `proc error_chain` signature `proc error_chain(error: Error) -> Iterator<Error> { give compiler_error_chain(error); }` example `error.error_chain(...)` stability `stable`

## `src/vitte/stdlib/std/event.vitl`

Stability: `stable`

- `form Event` signature `form Event { name: String payload: Vec<byte> timestamp: SystemTime }` example `event.Event` stability `stable`
- `form EventBus` signature `form EventBus { subscribers: Vec<String> }` example `event.EventBus` stability `stable`
- `form EventError` signature `form EventError { message: String }` example `event.EventError` stability `stable`
- `proc event` signature `proc event(name: String, payload: Vec<byte>) -> Event { give Event { name: name, payload: payload, timestamp: system_time_now() }; }` example `event.event(...)` stability `stable`
- `proc event_bus` signature `proc event_bus() -> EventBus { give EventBus { subscribers: vec_new<String>() }; }` example `event.event_bus(...)` stability `stable`
- `proc subscribe` signature `proc subscribe(bus: ref mut EventBus, topic: String) { vec_push<String>(&mut ((*bus).subscribers), topic); }` example `event.subscribe(...)` stability `stable`
- `proc publish` signature `proc publish(bus: ref EventBus, value: Event) -> Result<(), EventError> { give compiler_event_publish(bus, value); }` example `event.publish(...)` stability `stable`
- `proc subscriber_count` signature `proc subscriber_count(bus: EventBus) -> usize { give vec_len<String>(bus.subscribers); }` example `event.subscriber_count(...)` stability `stable`

## `src/vitte/stdlib/std/format.vitl`

Stability: `stable`

- `pick FormatBase` signature `pick FormatBase { Binary Octal Decimal HexLower HexUpper }` example `format.FormatBase` stability `stable`
- `form FormatSpec` signature `form FormatSpec { width: Option<usize> pad: rune base: FormatBase debug: bool }` example `format.FormatSpec` stability `stable`
- `form Display` signature `form Display<T> { fmt: proc(T, FormatSpec) -> String }` example `format.Display` stability `stable`
- `form Debug` signature `form Debug<T> { fmt_debug: proc(T) -> String }` example `format.Debug` stability `stable`
- `proc default_spec` signature `proc default_spec() -> FormatSpec { give FormatSpec { width: none<usize>(), pad: 32, base: FormatBase.Decimal, debug: false }; }` example `format.default_spec(...)` stability `stable`
- `proc format` signature `proc format<T>(value: T, spec: FormatSpec) -> String {` example `format.format(...)` stability `stable`
- `proc debug` signature `proc debug<T>(value: T) -> String { give compiler_debug_value<T>(value); }` example `format.debug(...)` stability `stable`
- `proc pad_left` signature `proc pad_left(text: String, width: usize, pad: rune) -> String {` example `format.pad_left(...)` stability `stable`
- `proc format_int` signature `proc format_int(value: i64, base: FormatBase) -> String {` example `format.format_int(...)` stability `stable`
- `proc format_uint` signature `proc format_uint(value: u64, base: FormatBase) -> String { give compiler_uint_to_base(value, base); }` example `format.format_uint(...)` stability `stable`

## `src/vitte/stdlib/std/fs.vitl`

Stability: `stable`

- `form Path` signature `form Path { text: string }` example `fs.Path` stability `stable`
- `form Permissions` signature `form Permissions { readonly: bool executable: bool }` example `fs.Permissions` stability `stable`
- `form Metadata` signature `form Metadata { is_file: bool is_dir: bool is_symlink: bool len: u64 permissions: Permissions }` example `fs.Metadata` stability `stable`
- `form FsError` signature `form FsError { code: int message: string path: Path }` example `fs.FsError` stability `stable`
- `form DirEntry` signature `form DirEntry { path: Path metadata: Metadata }` example `fs.DirEntry` stability `stable`
- `form WalkDir` signature `form WalkDir { root: Path follow_symlinks: bool }` example `fs.WalkDir` stability `stable`
- `proc path` signature `proc path(text: string) -> Path { give Path { text: text }; }` example `fs.path(...)` stability `stable`
- `proc fs_error` signature `proc fs_error(path_value: Path, message: string) -> FsError { give FsError { code: -1, message: message, path: path_value }; }` example `fs.fs_error(...)` stability `stable`
- `proc read` signature `proc read(path_value: Path) -> Result<Vec<byte>, FsError> { give compiler_backend_fs_read(path_value); }` example `fs.read(...)` stability `stable`
- `proc read_to_string` signature `proc read_to_string(path_value: Path) -> Result<String, FsError> { give compiler_backend_fs_read_to_string(path_value); }` example `fs.read_to_string(...)` stability `stable`
- `proc write` signature `proc write(path_value: Path, bytes: [byte]) -> Result<(), FsError> { give compiler_backend_fs_write(path_value, bytes); }` example `fs.write(...)` stability `stable`
- `proc create_dir_all` signature `proc create_dir_all(path_value: Path) -> Result<(), FsError> { give compiler_backend_fs_create_dir_all(path_value); }` example `fs.create_dir_all(...)` stability `stable`
- `proc create_dir` signature `proc create_dir(path_value: Path) -> Result<(), FsError> { give compiler_backend_fs_create_dir(path_value); }` example `fs.create_dir(...)` stability `stable`
- `proc remove_file` signature `proc remove_file(path_value: Path) -> Result<(), FsError> { give compiler_backend_fs_remove_file(path_value); }` example `fs.remove_file(...)` stability `stable`
- `proc remove_dir` signature `proc remove_dir(path_value: Path) -> Result<(), FsError> { give compiler_backend_fs_remove_dir(path_value); }` example `fs.remove_dir(...)` stability `stable`
- `proc rename` signature `proc rename(from: Path, to: Path) -> Result<(), FsError> { give compiler_backend_fs_rename(from, to); }` example `fs.rename(...)` stability `stable`
- `proc copy` signature `proc copy(from: Path, to: Path) -> Result<u64, FsError> { give compiler_backend_fs_copy(from, to); }` example `fs.copy(...)` stability `stable`
- `proc metadata` signature `proc metadata(path_value: Path) -> Result<Metadata, FsError> { give compiler_backend_fs_metadata(path_value); }` example `fs.metadata(...)` stability `stable`
- `proc symlink_metadata` signature `proc symlink_metadata(path_value: Path) -> Result<Metadata, FsError> { give compiler_backend_fs_symlink_metadata(path_value); }` example `fs.symlink_metadata(...)` stability `stable`
- `proc permissions` signature `proc permissions(path_value: Path) -> Result<Permissions, FsError> { give compiler_backend_fs_permissions(path_value); }` example `fs.permissions(...)` stability `stable`
- `proc set_permissions` signature `proc set_permissions(path_value: Path, value: Permissions) -> Result<(), FsError> { give compiler_backend_fs_set_permissions(path_value, value); }` example `fs.set_permissions(...)` stability `stable`
- `proc read_dir` signature `proc read_dir(path_value: Path) -> Result<Iterator<DirEntry>, FsError> { give compiler_backend_fs_read_dir(path_value); }` example `fs.read_dir(...)` stability `stable`
- `proc symlink` signature `proc symlink(original: Path, link: Path) -> Result<(), FsError> { give compiler_backend_fs_symlink(original, link); }` example `fs.symlink(...)` stability `stable`
- `proc read_link` signature `proc read_link(path_value: Path) -> Result<Path, FsError> { give compiler_backend_fs_read_link(path_value); }` example `fs.read_link(...)` stability `stable`
- `proc walk_dir` signature `proc walk_dir(root: Path) -> WalkDir { give WalkDir { root: root, follow_symlinks: false }; }` example `fs.walk_dir(...)` stability `stable`
- `proc walk_dir_following_symlinks` signature `proc walk_dir_following_symlinks(root: Path) -> WalkDir { give WalkDir { root: root, follow_symlinks: true }; }` example `fs.walk_dir_following_symlinks(...)` stability `stable`
- `proc walk_entries` signature `proc walk_entries(walk: WalkDir) -> Result<Iterator<DirEntry>, FsError> { give compiler_backend_fs_walk_entries(walk); }` example `fs.walk_entries(...)` stability `stable`
- `proc exists` signature `proc exists(path_value: Path) -> bool { give is_ok<Metadata, FsError>(metadata(path_value)); }` example `fs.exists(...)` stability `stable`

## `src/vitte/stdlib/std/glob.vitl`

Stability: `stable`

- `form GlobPattern` signature `form GlobPattern { text: String }` example `glob.GlobPattern` stability `stable`
- `form GlobOptions` signature `form GlobOptions { case_sensitive: bool recursive: bool }` example `glob.GlobOptions` stability `stable`
- `form GlobError` signature `form GlobError { message: String offset: usize }` example `glob.GlobError` stability `stable`
- `proc glob_pattern` signature `proc glob_pattern(text: String) -> Result<GlobPattern, GlobError> { give compiler_glob_pattern(text); }` example `glob.glob_pattern(...)` stability `stable`
- `proc glob_default_options` signature `proc glob_default_options() -> GlobOptions { give GlobOptions { case_sensitive: true, recursive: false }; }` example `glob.glob_default_options(...)` stability `stable`
- `proc glob_match` signature `proc glob_match(pattern: GlobPattern, path_value: PathBuf, options: GlobOptions) -> bool { give compiler_glob_match(pattern, path_value, options); }` example `glob.glob_match(...)` stability `stable`
- `proc glob_walk` signature `proc glob_walk(root: PathBuf, pattern: GlobPattern, options: GlobOptions) -> Result<Iterator<PathBuf>, GlobError> { give compiler_glob_walk(root, pattern, options); }` example `glob.glob_walk(...)` stability `stable`

## `src/vitte/stdlib/std/hash.vitl`

Stability: `stable`

- `form StableHasher` signature `form StableHasher { state: u64 }` example `hash.StableHasher` stability `stable`
- `form RandomHasher` signature `form RandomHasher { state: u64 }` example `hash.RandomHasher` stability `stable`
- `form SipHasher` signature `form SipHasher { k0: u64 k1: u64 state: u64 }` example `hash.SipHasher` stability `stable`
- `proc stable_hasher` signature `proc stable_hasher() -> StableHasher { give StableHasher { state: 1469598103934665603 }; }` example `hash.stable_hasher(...)` stability `stable`
- `proc random_hasher` signature `proc random_hasher() -> RandomHasher { give compiler_hash_random_hasher(); }` example `hash.random_hasher(...)` stability `stable`
- `proc sip_hasher` signature `proc sip_hasher(k0: u64, k1: u64) -> SipHasher { give SipHasher { k0: k0, k1: k1, state: 0 }; }` example `hash.sip_hasher(...)` stability `stable`
- `proc stable_write` signature `proc stable_write(hasher: ref mut StableHasher, bytes: [byte]) { compiler_hash_stable_write(hasher, bytes); }` example `hash.stable_write(...)` stability `stable`
- `proc random_write` signature `proc random_write(hasher: ref mut RandomHasher, bytes: [byte]) { compiler_hash_random_write(hasher, bytes); }` example `hash.random_write(...)` stability `stable`
- `proc sip_write` signature `proc sip_write(hasher: ref mut SipHasher, bytes: [byte]) { compiler_hash_sip_write(hasher, bytes); }` example `hash.sip_write(...)` stability `stable`
- `proc stable_finish` signature `proc stable_finish(hasher: StableHasher) -> u64 { give compiler_hash_stable_finish(hasher); }` example `hash.stable_finish(...)` stability `stable`
- `proc random_finish` signature `proc random_finish(hasher: RandomHasher) -> u64 { give compiler_hash_random_finish(hasher); }` example `hash.random_finish(...)` stability `stable`
- `proc sip_finish` signature `proc sip_finish(hasher: SipHasher) -> u64 { give compiler_hash_sip_finish(hasher); }` example `hash.sip_finish(...)` stability `stable`

## `src/vitte/stdlib/std/http.vitl`

Stability: `stable`

- `form HeaderMap` signature `form HeaderMap {` example `http.HeaderMap` stability `stable`
- `form Request` signature `form Request {` example `http.Request` stability `stable`
- `form Response` signature `form Response {` example `http.Response` stability `stable`
- `form HttpError` signature `form HttpError {` example `http.HttpError` stability `stable`
- `proc headers` signature `proc headers() -> HeaderMap { give HeaderMap { values: hashmap_new<String, String>() }; }` example `http.headers(...)` stability `stable`
- `proc header_insert` signature `proc header_insert(headers_value: ref mut HeaderMap, name: String, value: String) { hashmap_insert<String, String>(&mut ((*headers_value).values), name, value); }` example `http.header_insert(...)` stability `stable`
- `proc request` signature `proc request(method: String, url: Url) -> Request {` example `http.request(...)` stability `stable`
- `proc response` signature `proc response(status: u16) -> Response { give Response { status: status, headers: headers(), body: vec_new<byte>() }; }` example `http.response(...)` stability `stable`
- `proc with_body` signature `proc with_body(req: ref mut Request, body: Vec<byte>) { set (*req).body = body; }` example `http.with_body(...)` stability `stable`
- `proc follow_redirects` signature `proc follow_redirects(req: ref mut Request, max: usize) { set (*req).max_redirects = max; }` example `http.follow_redirects(...)` stability `stable`
- `proc http_get` signature `proc http_get(url: Url) -> Result<Response, HttpError> { give http_send(request(compiler_test_string("GET"), url)); }` example `http.http_get(...)` stability `stable`
- `proc http_send` signature `proc http_send(req: Request) -> Result<Response, HttpError> { give compiler_platform_http_send(req); }` example `http.http_send(...)` stability `stable`
- `proc http_post` signature `proc http_post(url: Url, body: Vec<byte>) -> Result<Response, HttpError> {` example `http.http_post(...)` stability `stable`

## `src/vitte/stdlib/std/io.vitl`

Stability: `stable`

- `form IoError` signature `form IoError { code: int message: string }` example `io.IoError` stability `stable`
- `form Reader` signature `form Reader { handle: int }` example `io.Reader` stability `stable`
- `form Writer` signature `form Writer { handle: int }` example `io.Writer` stability `stable`
- `form Read` signature `form Read { read: proc(ref mut Reader, ref mut [byte]) -> Result<usize, IoError> }` example `io.Read` stability `stable`
- `form Write` signature `form Write { write: proc(ref mut Writer, [byte]) -> Result<usize, IoError> flush: proc(ref mut Writer) -> Result<(), IoError> }` example `io.Write` stability `stable`
- `form Seek` signature `form Seek { seek: proc(ref mut Reader, i128) -> Result<usize, IoError> }` example `io.Seek` stability `stable`
- `form BufReader` signature `form BufReader { inner: Reader buffer: Vec<byte> position: usize }` example `io.BufReader` stability `stable`
- `form BufWriter` signature `form BufWriter { inner: Writer buffer: Vec<byte> }` example `io.BufWriter` stability `stable`
- `form Cursor` signature `form Cursor { data: Vec<byte> position: usize }` example `io.Cursor` stability `stable`
- `proc stdin` signature `proc stdin() -> Reader { give compiler_backend_stdio_stdin(); }` example `io.stdin(...)` stability `stable`
- `proc stdout` signature `proc stdout() -> Writer { give compiler_backend_stdio_stdout(); }` example `io.stdout(...)` stability `stable`
- `proc stderr` signature `proc stderr() -> Writer { give compiler_backend_stdio_stderr(); }` example `io.stderr(...)` stability `stable`
- `proc read_to_string` signature `proc read_to_string(reader: Reader) -> Result<String, IoError> { give compiler_backend_io_read_to_string(reader); }` example `io.read_to_string(...)` stability `stable`
- `proc read` signature `proc read(reader: ref mut Reader, buffer: ref mut [byte]) -> Result<usize, IoError> { give compiler_backend_io_read(reader, buffer); }` example `io.read(...)` stability `stable`
- `proc write` signature `proc write(writer: ref mut Writer, bytes: [byte]) -> Result<usize, IoError> { give compiler_backend_io_write(writer, bytes); }` example `io.write(...)` stability `stable`
- `proc write_all` signature `proc write_all(writer: Writer, bytes: [byte]) -> Result<(), IoError> {` example `io.write_all(...)` stability `stable`
- `proc write_string` signature `proc write_string(writer: Writer, text: string) -> Result<(), IoError> { give write_all(writer, compiler_string_bytes(text)); }` example `io.write_string(...)` stability `stable`
- `proc flush` signature `proc flush(writer: Writer) -> Result<(), IoError> { give compiler_backend_io_flush(writer); }` example `io.flush(...)` stability `stable`
- `proc buf_reader` signature `proc buf_reader(inner: Reader) -> BufReader { give BufReader { inner: inner, buffer: vec_new<byte>(), position: 0 }; }` example `io.buf_reader(...)` stability `stable`
- `proc buf_writer` signature `proc buf_writer(inner: Writer) -> BufWriter { give BufWriter { inner: inner, buffer: vec_new<byte>() }; }` example `io.buf_writer(...)` stability `stable`
- `proc cursor` signature `proc cursor(data: Vec<byte>) -> Cursor { give Cursor { data: data, position: 0 }; }` example `io.cursor(...)` stability `stable`
- `proc cursor_position` signature `proc cursor_position(cursor_value: Cursor) -> usize { give cursor_value.position; }` example `io.cursor_position(...)` stability `stable`
- `proc seek` signature `proc seek(reader: ref mut Reader, offset: i128) -> Result<usize, IoError> { give compiler_backend_io_seek(reader, offset); }` example `io.seek(...)` stability `stable`
- `proc buffered_read` signature `proc buffered_read(reader: ref mut BufReader, buffer: ref mut [byte]) -> Result<usize, IoError> {` example `io.buffered_read(...)` stability `stable`
- `proc buffered_write` signature `proc buffered_write(writer: ref mut BufWriter, bytes: [byte]) -> Result<usize, IoError> {` example `io.buffered_write(...)` stability `stable`
- `proc io_error` signature `proc io_error(code: int, message: string) -> IoError { give IoError { code: code, message: message }; }` example `io.io_error(...)` stability `stable`

## `src/vitte/stdlib/std/kernel.vitl`

Stability: `stable`

- `form KernelInfo` signature `form KernelInfo {` example `kernel.KernelInfo` stability `stable`
- `form KernelError` signature `form KernelError {` example `kernel.KernelError` stability `stable`
- `proc kernel_info` signature `proc kernel_info() -> Result<KernelInfo, KernelError> { give compiler_kernel_info(); }` example `kernel.kernel_info(...)` stability `stable`
- `proc kernel_name` signature `proc kernel_name() -> String { give compiler_kernel_name(); }` example `kernel.kernel_name(...)` stability `stable`
- `proc kernel_release` signature `proc kernel_release() -> String { give compiler_kernel_release(); }` example `kernel.kernel_release(...)` stability `stable`
- `proc page_size` signature `proc page_size() -> usize { give compiler_kernel_page_size(); }` example `kernel.page_size(...)` stability `stable`
- `proc cpu_count` signature `proc cpu_count() -> usize { give compiler_kernel_cpu_count(); }` example `kernel.cpu_count(...)` stability `stable`

## `src/vitte/stdlib/std/locale.vitl`

Stability: `stable`

- `form Locale` signature `form Locale { language: String region: Option<String> }` example `locale.Locale` stability `stable`
- `form LocaleError` signature `form LocaleError { message: String }` example `locale.LocaleError` stability `stable`
- `proc locale_parse` signature `proc locale_parse(text: Utf8View) -> Result<Locale, LocaleError> { give compiler_locale_parse(text); }` example `locale.locale_parse(...)` stability `stable`
- `proc locale_current` signature `proc locale_current() -> Option<Locale> { give compiler_locale_current(); }` example `locale.locale_current(...)` stability `stable`
- `proc locale_to_string` signature `proc locale_to_string(value: Locale) -> String { give compiler_locale_to_string(value); }` example `locale.locale_to_string(...)` stability `stable`
- `proc locale_language` signature `proc locale_language(value: Locale) -> String { give value.language; }` example `locale.locale_language(...)` stability `stable`
- `proc locale_region` signature `proc locale_region(value: Locale) -> Option<String> { give value.region; }` example `locale.locale_region(...)` stability `stable`

## `src/vitte/stdlib/std/log.vitl`

Stability: `stable`

- `pick LogLevel` signature `pick LogLevel { Trace Debug Info Warn Error }` example `log.LogLevel` stability `stable`
- `form LogRecord` signature `form LogRecord { level: LogLevel target: String message: String }` example `log.LogRecord` stability `stable`
- `form Logger` signature `form Logger { sink: Writer min_level: LogLevel compact: bool }` example `log.Logger` stability `stable`
- `proc logger` signature `proc logger(sink: Writer, min_level: LogLevel) -> Logger { give Logger { sink: sink, min_level: min_level, compact: true }; }` example `log.logger(...)` stability `stable`
- `proc log` signature `proc log(logger_value: ref mut Logger, record: LogRecord) { compiler_log_write(logger_value, record); }` example `log.log(...)` stability `stable`
- `proc trace` signature `proc trace(logger_value: ref mut Logger, message: String) { log(logger_value, LogRecord { level: LogLevel.Trace, target: compiler_log_default_target(), message: message }); }` example `log.trace(...)` stability `stable`
- `proc info` signature `proc info(logger_value: ref mut Logger, message: String) { log(logger_value, LogRecord { level: LogLevel.Info, target: compiler_log_default_target(), message: message }); }` example `log.info(...)` stability `stable`
- `proc warn` signature `proc warn(logger_value: ref mut Logger, message: String) { log(logger_value, LogRecord { level: LogLevel.Warn, target: compiler_log_default_target(), message: message }); }` example `log.warn(...)` stability `stable`
- `proc error` signature `proc error(logger_value: ref mut Logger, message: String) { log(logger_value, LogRecord { level: LogLevel.Error, target: compiler_log_default_target(), message: message }); }` example `log.error(...)` stability `stable`

## `src/vitte/stdlib/std/metrics.vitl`

Stability: `stable`

- `form Counter` signature `form Counter { name: String value: u64 }` example `metrics.Counter` stability `stable`
- `form Gauge` signature `form Gauge { name: String value: f64 }` example `metrics.Gauge` stability `stable`
- `form Histogram` signature `form Histogram { name: String buckets: Vec<f64> }` example `metrics.Histogram` stability `stable`
- `proc counter` signature `proc counter(name: String) -> Counter { give Counter { name: name, value: 0 }; }` example `metrics.counter(...)` stability `stable`
- `proc counter_inc` signature `proc counter_inc(value: ref mut Counter, amount: u64) { set (*value).value = (*value).value + amount; }` example `metrics.counter_inc(...)` stability `stable`
- `proc gauge` signature `proc gauge(name: String, value: f64) -> Gauge { give Gauge { name: name, value: value }; }` example `metrics.gauge(...)` stability `stable`
- `proc histogram` signature `proc histogram(name: String, buckets: Vec<f64>) -> Histogram { give Histogram { name: name, buckets: buckets }; }` example `metrics.histogram(...)` stability `stable`
- `proc metrics_render_counter` signature `proc metrics_render_counter(value: Counter) -> String {` example `metrics.metrics_render_counter(...)` stability `stable`

## `src/vitte/stdlib/std/mime.vitl`

Stability: `stable`

- `form Mime` signature `form Mime {` example `mime.Mime` stability `stable`
- `form MimeError` signature `form MimeError {` example `mime.MimeError` stability `stable`
- `proc mime_parse` signature `proc mime_parse(text: Utf8View) -> Result<Mime, MimeError> { give compiler_mime_parse(text); }` example `mime.mime_parse(...)` stability `stable`
- `proc mime_to_string` signature `proc mime_to_string(value: Mime) -> String { give compiler_mime_to_string(value); }` example `mime.mime_to_string(...)` stability `stable`
- `proc mime_text_plain` signature `proc mime_text_plain() -> Mime {` example `mime.mime_text_plain(...)` stability `stable`
- `proc mime_application_json` signature `proc mime_application_json() -> Mime {` example `mime.mime_application_json(...)` stability `stable`

## `src/vitte/stdlib/std/net.vitl`

Stability: `stable`

- `form IpAddr` signature `form IpAddr { text: String }` example `net.IpAddr` stability `stable`
- `form SocketAddr` signature `form SocketAddr { ip: IpAddr port: u16 }` example `net.SocketAddr` stability `stable`
- `form TcpStream` signature `form TcpStream { handle: int }` example `net.TcpStream` stability `stable`
- `form TcpListener` signature `form TcpListener { handle: int }` example `net.TcpListener` stability `stable`
- `form UdpSocket` signature `form UdpSocket { handle: int }` example `net.UdpSocket` stability `stable`
- `form Timeout` signature `form Timeout { millis: u64 }` example `net.Timeout` stability `stable`
- `form NetError` signature `form NetError { message: String }` example `net.NetError` stability `stable`
- `proc net_error` signature `proc net_error(message: String) -> NetError { give NetError { message: message }; }` example `net.net_error(...)` stability `stable`
- `proc ip_addr` signature `proc ip_addr(text: String) -> Result<IpAddr, NetError> {` example `net.ip_addr(...)` stability `stable`
- `proc socket_addr` signature `proc socket_addr(ip: IpAddr, port: u16) -> SocketAddr { give SocketAddr { ip: ip, port: port }; }` example `net.socket_addr(...)` stability `stable`
- `proc dns_lookup` signature `proc dns_lookup(name: String) -> Result<Vec<IpAddr>, NetError> { give compiler_platform_dns_lookup(name); }` example `net.dns_lookup(...)` stability `stable`
- `proc tcp_connect` signature `proc tcp_connect(addr: SocketAddr) -> Result<TcpStream, NetError> { give compiler_platform_tcp_connect(addr); }` example `net.tcp_connect(...)` stability `stable`
- `proc tcp_connect_timeout` signature `proc tcp_connect_timeout(addr: SocketAddr, timeout: Timeout) -> Result<TcpStream, NetError> { give compiler_platform_tcp_connect_timeout(addr, timeout); }` example `net.tcp_connect_timeout(...)` stability `stable`
- `proc tcp_bind` signature `proc tcp_bind(addr: SocketAddr) -> Result<TcpListener, NetError> { give compiler_platform_tcp_bind(addr); }` example `net.tcp_bind(...)` stability `stable`
- `proc tcp_accept` signature `proc tcp_accept(listener: ref TcpListener) -> Result<TcpStream, NetError> { give compiler_platform_tcp_accept(listener); }` example `net.tcp_accept(...)` stability `stable`
- `proc tcp_read` signature `proc tcp_read(stream: ref TcpStream, buffer: ref mut [byte]) -> Result<usize, NetError> { give compiler_platform_tcp_read(stream, buffer); }` example `net.tcp_read(...)` stability `stable`
- `proc tcp_write` signature `proc tcp_write(stream: ref TcpStream, bytes: [byte]) -> Result<usize, NetError> { give compiler_platform_tcp_write(stream, bytes); }` example `net.tcp_write(...)` stability `stable`
- `proc udp_bind` signature `proc udp_bind(addr: SocketAddr) -> Result<UdpSocket, NetError> { give compiler_platform_udp_bind(addr); }` example `net.udp_bind(...)` stability `stable`
- `proc udp_send_to` signature `proc udp_send_to(socket: ref UdpSocket, bytes: [byte], addr: SocketAddr) -> Result<usize, NetError> { give compiler_platform_udp_send_to(socket, bytes, addr); }` example `net.udp_send_to(...)` stability `stable`
- `proc udp_recv_from` signature `proc udp_recv_from(socket: ref UdpSocket, buffer: ref mut [byte]) -> Result<(usize, SocketAddr), NetError> { give compiler_platform_udp_recv_from(socket, buffer); }` example `net.udp_recv_from(...)` stability `stable`
- `proc set_timeout` signature `proc set_timeout(socket: ref UdpSocket, timeout: Timeout) -> Result<(), NetError> { give compiler_platform_udp_set_timeout(socket, timeout); }` example `net.set_timeout(...)` stability `stable`
- `proc timeout_millis` signature `proc timeout_millis(value: u64) -> Timeout { give Timeout { millis: value }; }` example `net.timeout_millis(...)` stability `stable`

## `src/vitte/stdlib/std/parse.vitl`

Stability: `stable`

- `form ParseError` signature `form ParseError { message: String offset: usize }` example `parse.ParseError` stability `stable`
- `proc parse_bool` signature `proc parse_bool(text: Utf8View) -> Result<bool, ParseError> {` example `parse.parse_bool(...)` stability `stable`
- `proc parse_i64` signature `proc parse_i64(text: Utf8View, base: u32) -> Result<i64, ParseError> {` example `parse.parse_i64(...)` stability `stable`
- `proc parse_u64` signature `proc parse_u64(text: Utf8View, base: u32) -> Result<u64, ParseError> {` example `parse.parse_u64(...)` stability `stable`
- `proc parse_f64` signature `proc parse_f64(text: Utf8View) -> Result<f64, ParseError> {` example `parse.parse_f64(...)` stability `stable`
- `proc parse_utf8` signature `proc parse_utf8(bytes: [byte]) -> Result<Utf8View, ParseError> {` example `parse.parse_utf8(...)` stability `stable`
- `proc parse_error` signature `proc parse_error(message: String, offset: usize) -> ParseError { give ParseError { message: message, offset: offset }; }` example `parse.parse_error(...)` stability `stable`

## `src/vitte/stdlib/std/path.vitl`

Stability: `stable`

- `pick PathStyle` signature `pick PathStyle { Unix Windows }` example `path.PathStyle` stability `stable`
- `pick ComponentKind` signature `pick ComponentKind { Root Prefix Current Parent Normal }` example `path.ComponentKind` stability `stable`
- `form Component` signature `form Component { kind: ComponentKind text: String }` example `path.Component` stability `stable`
- `form PathBuf` signature `form PathBuf { value: String absolute: bool style: PathStyle }` example `path.PathBuf` stability `stable`
- `proc path_buf` signature `proc path_buf(text: String) -> PathBuf {` example `path.path_buf(...)` stability `stable`
- `proc unix_path` signature `proc unix_path(text: String) -> PathBuf { give PathBuf { value: text, absolute: compiler_unix_path_is_absolute(text), style: PathStyle.Unix }; }` example `path.unix_path(...)` stability `stable`
- `proc windows_path` signature `proc windows_path(text: String) -> PathBuf { give PathBuf { value: text, absolute: compiler_windows_path_is_absolute(text), style: PathStyle.Windows }; }` example `path.windows_path(...)` stability `stable`
- `proc components` signature `proc components(path_value: PathBuf) -> Iterator<Component> { give compiler_path_components_text(path_value.value, path_value.style); }` example `path.components(...)` stability `stable`
- `proc normalize` signature `proc normalize(path_value: PathBuf) -> PathBuf {` example `path.normalize(...)` stability `stable`
- `proc normalize_strict` signature `proc normalize_strict(path_value: PathBuf) -> Result<PathBuf, String> {` example `path.normalize_strict(...)` stability `stable`
- `proc is_absolute` signature `proc is_absolute(path_value: PathBuf) -> bool { give path_value.absolute; }` example `path.is_absolute(...)` stability `stable`
- `proc is_relative` signature `proc is_relative(path_value: PathBuf) -> bool { give not path_value.absolute; }` example `path.is_relative(...)` stability `stable`
- `proc join` signature `proc join(base: PathBuf, child: PathBuf) -> PathBuf {` example `path.join(...)` stability `stable`
- `proc parent` signature `proc parent(path_value: PathBuf) -> Option<PathBuf> {` example `path.parent(...)` stability `stable`
- `proc file_name` signature `proc file_name(path_value: PathBuf) -> Option<String> { give compiler_path_file_name_text(path_value.value, path_value.style); }` example `path.file_name(...)` stability `stable`
- `proc extension` signature `proc extension(path_value: PathBuf) -> Option<String> { give compiler_path_extension_text(path_value.value, path_value.style); }` example `path.extension(...)` stability `stable`
- `proc with_extension` signature `proc with_extension(path_value: PathBuf, ext: String) -> PathBuf {` example `path.with_extension(...)` stability `stable`

## `src/vitte/stdlib/std/percent_encoding.vitl`

Stability: `stable`

- `form PercentEncodeSet` signature `form PercentEncodeSet {` example `percent_encoding.PercentEncodeSet` stability `stable`
- `form PercentDecodeError` signature `form PercentDecodeError {` example `percent_encoding.PercentDecodeError` stability `stable`
- `proc percent_encode_set_component` signature `proc percent_encode_set_component() -> PercentEncodeSet {` example `percent_encoding.percent_encode_set_component(...)` stability `stable`
- `proc percent_encode` signature `proc percent_encode(text: Utf8View, set: PercentEncodeSet) -> String { give compiler_percent_encode(text, set); }` example `percent_encoding.percent_encode(...)` stability `stable`
- `proc percent_decode` signature `proc percent_decode(text: Utf8View) -> Result<String, PercentDecodeError> { give compiler_percent_decode(text); }` example `percent_encoding.percent_decode(...)` stability `stable`

## `src/vitte/stdlib/std/process.vitl`

Stability: `stable`

- `form ExitStatus` signature `form ExitStatus { code: int success: bool }` example `process.ExitStatus` stability `stable`
- `form Stdio` signature `form Stdio { mode: string }` example `process.Stdio` stability `stable`
- `form Pipe` signature `form Pipe { reader: Reader writer: Writer }` example `process.Pipe` stability `stable`
- `form Child` signature `form Child { id: u64 stdin: Option<Writer> stdout: Option<Reader> stderr: Option<Reader> }` example `process.Child` stability `stable`
- `form Output` signature `form Output { status: ExitStatus stdout: Vec<byte> stderr: Vec<byte> }` example `process.Output` stability `stable`
- `form Command` signature `form Command { program: String args: Vec<String> env: Vec<(String, String)> stdin: Stdio stdout: Stdio stderr: Stdio cwd: Option<Path> }` example `process.Command` stability `stable`
- `form ProcessError` signature `form ProcessError { code: int message: string }` example `process.ProcessError` stability `stable`
- `proc command` signature `proc command(program: String) -> Command {` example `process.command(...)` stability `stable`
- `proc arg` signature `proc arg(cmd: ref mut Command, value: String) { vec_push<String>(&mut ((*cmd).args), value); }` example `process.arg(...)` stability `stable`
- `proc env` signature `proc env(cmd: ref mut Command, name: String, value: String) { vec_push<(String, String)>(&mut ((*cmd).env), (name, value)); }` example `process.env(...)` stability `stable`
- `proc env_remove` signature `proc env_remove(cmd: ref mut Command, name: String) { compiler_backend_process_env_remove_from_vec(&mut ((*cmd).env), name); }` example `process.env_remove(...)` stability `stable`
- `proc current_dir` signature `proc current_dir(cmd: ref mut Command, path_value: Path) { set (*cmd).cwd = some<Path>(path_value); }` example `process.current_dir(...)` stability `stable`
- `proc stdin_piped` signature `proc stdin_piped() -> Stdio { give Stdio { mode: "piped" }; }` example `process.stdin_piped(...)` stability `stable`
- `proc stdout_piped` signature `proc stdout_piped() -> Stdio { give Stdio { mode: "piped" }; }` example `process.stdout_piped(...)` stability `stable`
- `proc stderr_piped` signature `proc stderr_piped() -> Stdio { give Stdio { mode: "piped" }; }` example `process.stderr_piped(...)` stability `stable`
- `proc inherit` signature `proc inherit() -> Stdio { give Stdio { mode: "inherit" }; }` example `process.inherit(...)` stability `stable`
- `proc null` signature `proc null() -> Stdio { give Stdio { mode: "null" }; }` example `process.null(...)` stability `stable`
- `proc pipe` signature `proc pipe() -> Result<Pipe, ProcessError> { give compiler_backend_process_pipe(); }` example `process.pipe(...)` stability `stable`
- `proc stdin_mode` signature `proc stdin_mode(cmd: ref mut Command, mode: Stdio) { set (*cmd).stdin = mode; }` example `process.stdin_mode(...)` stability `stable`
- `proc stdout_mode` signature `proc stdout_mode(cmd: ref mut Command, mode: Stdio) { set (*cmd).stdout = mode; }` example `process.stdout_mode(...)` stability `stable`
- `proc stderr_mode` signature `proc stderr_mode(cmd: ref mut Command, mode: Stdio) { set (*cmd).stderr = mode; }` example `process.stderr_mode(...)` stability `stable`
- `proc spawn` signature `proc spawn(cmd: Command) -> Result<Child, ProcessError> { give compiler_backend_process_spawn(cmd); }` example `process.spawn(...)` stability `stable`
- `proc wait` signature `proc wait(child: Child) -> Result<ExitStatus, ProcessError> { give compiler_backend_process_wait(child); }` example `process.wait(...)` stability `stable`
- `proc status` signature `proc status(cmd: Command) -> Result<ExitStatus, ProcessError> {` example `process.status(...)` stability `stable`
- `proc output` signature `proc output(cmd: Command) -> Result<Output, ProcessError> { give compiler_backend_process_output(cmd); }` example `process.output(...)` stability `stable`
- `proc exit` signature `proc exit(code: int) -> never { give compiler_backend_process_exit(code); }` example `process.exit(...)` stability `stable`
- `proc exit_status` signature `proc exit_status(code: int) -> ExitStatus { give ExitStatus { code: code, success: code == 0 }; }` example `process.exit_status(...)` stability `stable`

## `src/vitte/stdlib/std/random.vitl`

Stability: `stable`

- `form Prng` signature `form Prng { state: u64 }` example `random.Prng` stability `stable`
- `form RandomError` signature `form RandomError { message: String }` example `random.RandomError` stability `stable`
- `proc prng` signature `proc prng(seed: u64) -> Prng {` example `random.prng(...)` stability `stable`
- `proc next_u64` signature `proc next_u64(rng: ref mut Prng) -> u64 {` example `random.next_u64(...)` stability `stable`
- `proc next_bool` signature `proc next_bool(rng: ref mut Prng) -> bool { give (next_u64(rng) & 1) == 1; }` example `random.next_bool(...)` stability `stable`
- `proc range_u64` signature `proc range_u64(rng: ref mut Prng, upper: u64) -> u64 { give next_u64(rng) % upper; }` example `random.range_u64(...)` stability `stable`
- `proc os_entropy` signature `proc os_entropy(buffer: ref mut [byte]) -> Result<(), RandomError> {` example `random.os_entropy(...)` stability `stable`
- `proc random_f64` signature `proc random_f64(rng: ref mut Prng) -> f64 { give compiler_u64_to_unit_f64(next_u64(rng)); }` example `random.random_f64(...)` stability `stable`

## `src/vitte/stdlib/std/serialization.vitl`

Stability: `stable`

- `pick JsonTokenKind` signature `pick JsonTokenKind { Null Bool Number String Array Object }` example `serialization.JsonTokenKind` stability `stable`
- `form Encode` signature `form Encode<T> {` example `serialization.Encode` stability `stable`
- `form Decode` signature `form Decode<T> {` example `serialization.Decode` stability `stable`
- `form SerializationError` signature `form SerializationError {` example `serialization.SerializationError` stability `stable`
- `form Encoder` signature `form Encoder {` example `serialization.Encoder` stability `stable`
- `form Decoder` signature `form Decoder {` example `serialization.Decoder` stability `stable`
- `form JsonValue` signature `form JsonValue {` example `serialization.JsonValue` stability `stable`
- `form JsonField` signature `form JsonField { name: String value: JsonValue }` example `serialization.JsonField` stability `stable`
- `proc serialization_error` signature `proc serialization_error(message: String, offset: usize) -> SerializationError {` example `serialization.serialization_error(...)` stability `stable`
- `proc json_encoder` signature `proc json_encoder() -> Encoder {` example `serialization.json_encoder(...)` stability `stable`
- `proc json_decoder` signature `proc json_decoder(input: Utf8View) -> Decoder {` example `serialization.json_decoder(...)` stability `stable`
- `proc encode_json` signature `proc encode_json<T>(value: T) -> Result<String, SerializationError> {` example `serialization.encode_json(...)` stability `stable`
- `proc decode_json` signature `proc decode_json<T>(input: Utf8View) -> Result<T, SerializationError> {` example `serialization.decode_json(...)` stability `stable`
- `proc json_value` signature `proc json_value(input: Utf8View) -> Result<JsonValue, SerializationError> {` example `serialization.json_value(...)` stability `stable`
- `proc write_json_string` signature `proc write_json_string(encoder: ref mut Encoder, value: String) -> Result<(), SerializationError> {` example `serialization.write_json_string(...)` stability `stable`
- `proc write_json_field` signature `proc write_json_field<T>(encoder: ref mut Encoder, name: String, value: T) -> Result<(), SerializationError> {` example `serialization.write_json_field(...)` stability `stable`
- `proc json_null` signature `proc json_null() -> JsonValue { give JsonValue { kind: JsonTokenKind.Null, text: compiler_test_string("null") }; }` example `serialization.json_null(...)` stability `stable`
- `proc json_bool` signature `proc json_bool(value: bool) -> JsonValue {` example `serialization.json_bool(...)` stability `stable`
- `proc json_string` signature `proc json_string(value: String) -> JsonValue { give JsonValue { kind: JsonTokenKind.String, text: compiler_json_quote(value) }; }` example `serialization.json_string(...)` stability `stable`
- `proc encode_derived` signature `proc encode_derived<T>(value: T, encoder: ref mut Encoder, fields: proc(T, ref mut Encoder) -> Result<(), SerializationError>) -> Result<String, SerializationError> {` example `serialization.encode_derived(...)` stability `stable`
- `proc decode_derived` signature `proc decode_derived<T>(decoder: ref mut Decoder, build: proc(JsonValue) -> Result<T, SerializationError>) -> Result<T, SerializationError> {` example `serialization.decode_derived(...)` stability `stable`

## `src/vitte/stdlib/std/semver.vitl`

Stability: `stable`

- `form Version` signature `form Version {` example `semver.Version` stability `stable`
- `form VersionReq` signature `form VersionReq {` example `semver.VersionReq` stability `stable`
- `form SemverError` signature `form SemverError {` example `semver.SemverError` stability `stable`
- `proc version` signature `proc version(major: u64, minor: u64, patch: u64) -> Version {` example `semver.version(...)` stability `stable`
- `proc semver_error` signature `proc semver_error(message: String, offset: usize) -> SemverError { give SemverError { message: message, offset: offset }; }` example `semver.semver_error(...)` stability `stable`
- `proc semver_parse` signature `proc semver_parse(text: Utf8View) -> Result<Version, SemverError> {` example `semver.semver_parse(...)` stability `stable`
- `proc semver_req_parse` signature `proc semver_req_parse(text: Utf8View) -> Result<VersionReq, SemverError> {` example `semver.semver_req_parse(...)` stability `stable`
- `proc semver_matches` signature `proc semver_matches(req: VersionReq, value: Version) -> bool { give compiler_semver_req_matches_parts(req.text, value.major, value.minor, value.patch); }` example `semver.semver_matches(...)` stability `stable`
- `proc semver_compare` signature `proc semver_compare(left: Version, right: Version) -> Ordering {` example `semver.semver_compare(...)` stability `stable`

## `src/vitte/stdlib/std/signal.vitl`

Stability: `stable`

- `form Signal` signature `form Signal { number: int name: String }` example `signal.Signal` stability `stable`
- `form SignalHandler` signature `form SignalHandler { signal: Signal }` example `signal.SignalHandler` stability `stable`
- `form SignalError` signature `form SignalError { message: String }` example `signal.SignalError` stability `stable`
- `proc signal` signature `proc signal(number: int, name: String) -> Signal { give Signal { number: number, name: name }; }` example `signal.signal(...)` stability `stable`
- `proc signal_int` signature `proc signal_int() -> Signal { give signal(2, compiler_test_string("INT")); }` example `signal.signal_int(...)` stability `stable`
- `proc signal_term` signature `proc signal_term() -> Signal { give signal(15, compiler_test_string("TERM")); }` example `signal.signal_term(...)` stability `stable`
- `proc signal_install` signature `proc signal_install(sig: Signal, handler: proc(Signal)) -> Result<SignalHandler, SignalError> { give compiler_signal_install(sig, handler); }` example `signal.signal_install(...)` stability `stable`
- `proc signal_raise` signature `proc signal_raise(sig: Signal) -> Result<(), SignalError> { give compiler_signal_raise(sig); }` example `signal.signal_raise(...)` stability `stable`

## `src/vitte/stdlib/std/sync.vitl`

Stability: `stable`

- `form Mutex` signature `form Mutex<T> { state: compiler_mutex_state value: T }` example `sync.Mutex` stability `stable`
- `form RwLock` signature `form RwLock<T> { state: compiler_rwlock_state value: T }` example `sync.RwLock` stability `stable`
- `form Once` signature `form Once { state: compiler_once_state }` example `sync.Once` stability `stable`
- `form Condvar` signature `form Condvar { state: compiler_condvar_state }` example `sync.Condvar` stability `stable`
- `form Atomic` signature `form Atomic<T> { value: T }` example `sync.Atomic` stability `stable`
- `form MutexGuard` signature `form MutexGuard<T> { value: ref mut T }` example `sync.MutexGuard` stability `stable`
- `form RwReadGuard` signature `form RwReadGuard<T> { value: ref T }` example `sync.RwReadGuard` stability `stable`
- `form RwWriteGuard` signature `form RwWriteGuard<T> { value: ref mut T }` example `sync.RwWriteGuard` stability `stable`
- `proc mutex` signature `proc mutex<T>(value: T) -> Mutex<T> { give compiler_mutex_new<T>(value); }` example `sync.mutex(...)` stability `stable`
- `proc mutex_lock` signature `proc mutex_lock<T>(mutex_value: ref Mutex<T>) -> ref mut T { give compiler_mutex_lock<T>(mutex_value); }` example `sync.mutex_lock(...)` stability `stable`
- `proc lock` signature `proc lock<T>(mutex_value: ref Mutex<T>) -> MutexGuard<T> { give MutexGuard<T> { value: mutex_lock<T>(mutex_value) }; }` example `sync.lock(...)` stability `stable`
- `proc mutex_unlock` signature `proc mutex_unlock<T>(mutex_value: ref Mutex<T>) { compiler_mutex_unlock<T>(mutex_value); }` example `sync.mutex_unlock(...)` stability `stable`
- `proc rwlock` signature `proc rwlock<T>(value: T) -> RwLock<T> { give compiler_rwlock_new<T>(value); }` example `sync.rwlock(...)` stability `stable`
- `proc read` signature `proc read<T>(lock: ref RwLock<T>) -> ref T { give compiler_rwlock_read<T>(lock); }` example `sync.read(...)` stability `stable`
- `proc write` signature `proc write<T>(lock: ref RwLock<T>) -> ref mut T { give compiler_rwlock_write<T>(lock); }` example `sync.write(...)` stability `stable`
- `proc read_guard` signature `proc read_guard<T>(lock: ref RwLock<T>) -> RwReadGuard<T> { give RwReadGuard<T> { value: read<T>(lock) }; }` example `sync.read_guard(...)` stability `stable`
- `proc write_guard` signature `proc write_guard<T>(lock: ref RwLock<T>) -> RwWriteGuard<T> { give RwWriteGuard<T> { value: write<T>(lock) }; }` example `sync.write_guard(...)` stability `stable`
- `proc once` signature `proc once() -> Once { give compiler_once_new(); }` example `sync.once(...)` stability `stable`
- `proc call_once` signature `proc call_once(once_value: ref Once, f: proc()) { compiler_once_call(once_value, f); }` example `sync.call_once(...)` stability `stable`
- `proc condvar` signature `proc condvar() -> Condvar { give compiler_condvar_new(); }` example `sync.condvar(...)` stability `stable`
- `proc wait` signature `proc wait<T>(cond: ref Condvar, mutex_value: ref Mutex<T>) { compiler_condvar_wait<T>(cond, mutex_value); }` example `sync.wait(...)` stability `stable`
- `proc notify_one` signature `proc notify_one(cond: ref Condvar) { compiler_condvar_notify_one(cond); }` example `sync.notify_one(...)` stability `stable`
- `proc notify_all` signature `proc notify_all(cond: ref Condvar) { compiler_condvar_notify_all(cond); }` example `sync.notify_all(...)` stability `stable`
- `proc atomic` signature `proc atomic<T>(value: T) -> Atomic<T> { give Atomic<T> { value: value }; }` example `sync.atomic(...)` stability `stable`
- `proc atomic_load` signature `proc atomic_load<T>(value: ref Atomic<T>) -> T { give (*value).value; }` example `sync.atomic_load(...)` stability `stable`
- `proc atomic_store` signature `proc atomic_store<T>(value: ref Atomic<T>, next: T) { set (*value).value = next; }` example `sync.atomic_store(...)` stability `stable`
- `proc atomic_compare_exchange` signature `proc atomic_compare_exchange<T>(value: ref Atomic<T>, current: T, next: T) -> Result<T, T> {` example `sync.atomic_compare_exchange(...)` stability `stable`

## `src/vitte/stdlib/std/tempfile.vitl`

Stability: `stable`

- `form TempFile` signature `form TempFile { path: Path delete_on_drop: bool }` example `tempfile.TempFile` stability `stable`
- `form TempDir` signature `form TempDir { path: Path delete_on_drop: bool }` example `tempfile.TempDir` stability `stable`
- `form TempError` signature `form TempError { message: String }` example `tempfile.TempError` stability `stable`
- `proc temp_file` signature `proc temp_file(prefix: String) -> Result<TempFile, TempError> { give compiler_temp_file(prefix); }` example `tempfile.temp_file(...)` stability `stable`
- `proc temp_dir` signature `proc temp_dir(prefix: String) -> Result<TempDir, TempError> { give compiler_temp_dir(prefix); }` example `tempfile.temp_dir(...)` stability `stable`
- `proc temp_path` signature `proc temp_path(prefix: String) -> Path { give compiler_temp_path(prefix); }` example `tempfile.temp_path(...)` stability `stable`
- `proc persist_file` signature `proc persist_file(file: TempFile, target: Path) -> Result<Path, TempError> { give compiler_temp_persist_file(file, target); }` example `tempfile.persist_file(...)` stability `stable`
- `proc persist_dir` signature `proc persist_dir(dir: TempDir, target: Path) -> Result<Path, TempError> { give compiler_temp_persist_dir(dir, target); }` example `tempfile.persist_dir(...)` stability `stable`

## `src/vitte/stdlib/std/testing.vitl`

Stability: `stable`

- `form TestError` signature `form TestError { message: String file: string line: u32 }` example `testing.TestError` stability `stable`
- `form Fixture` signature `form Fixture<T> { setup: proc() -> T teardown: proc(T) }` example `testing.Fixture` stability `stable`
- `form Snapshot` signature `form Snapshot { name: String value: String }` example `testing.Snapshot` stability `stable`
- `form Case` signature `form Case<T> { name: String value: T }` example `testing.Case` stability `stable`
- `form TestCase` signature `form TestCase { name: String run: proc() -> Result<(), TestError> }` example `testing.TestCase` stability `stable`
- `form TestReport` signature `form TestReport { total: usize passed: usize failed: usize failures: Vec<TestError> }` example `testing.TestReport` stability `stable`
- `proc test_error` signature `proc test_error(message: String) -> TestError { give TestError { message: message, file: "stdlib", line: 0 }; }` example `testing.test_error(...)` stability `stable`
- `proc assert_true` signature `proc assert_true(value: bool, message: String) -> Result<(), TestError> {` example `testing.assert_true(...)` stability `stable`
- `proc assert_eq` signature `proc assert_eq<T>(left: T, right: T, message: String) -> Result<(), TestError> {` example `testing.assert_eq(...)` stability `stable`
- `proc snapshot` signature `proc snapshot(name: String, value: String) -> Snapshot { give Snapshot { name: name, value: value }; }` example `testing.snapshot(...)` stability `stable`
- `proc assert_snapshot` signature `proc assert_snapshot(value: Snapshot) -> Result<(), TestError> {` example `testing.assert_snapshot(...)` stability `stable`
- `proc parameterized` signature `proc parameterized<T>(cases: Vec<Case<T>>, test: proc(T) -> Result<(), TestError>) -> Result<(), TestError> {` example `testing.parameterized(...)` stability `stable`
- `proc run_tests` signature `proc run_tests(cases: Vec<TestCase>) -> TestReport {` example `testing.run_tests(...)` stability `stable`

## `src/vitte/stdlib/std/thread.vitl`

Stability: `stable`

- `form ThreadId` signature `form ThreadId { value: u64 }` example `thread.ThreadId` stability `stable`
- `form JoinHandle` signature `form JoinHandle<T> { id: ThreadId }` example `thread.JoinHandle` stability `stable`
- `form ThreadError` signature `form ThreadError { message: String }` example `thread.ThreadError` stability `stable`
- `form ThreadBuilder` signature `form ThreadBuilder { name: Option<String> stack_size: Option<usize> }` example `thread.ThreadBuilder` stability `stable`
- `proc current_id` signature `proc current_id() -> ThreadId { give compiler_backend_thread_current_id(); }` example `thread.current_id(...)` stability `stable`
- `proc spawn` signature `proc spawn<T>(f: proc() -> T) -> Result<JoinHandle<T>, ThreadError> { give compiler_backend_thread_spawn<T>(f); }` example `thread.spawn(...)` stability `stable`
- `proc thread_builder` signature `proc thread_builder() -> ThreadBuilder { give ThreadBuilder { name: none<String>(), stack_size: none<usize>() }; }` example `thread.thread_builder(...)` stability `stable`
- `proc name` signature `proc name(builder: ref mut ThreadBuilder, value: String) { set (*builder).name = some<String>(value); }` example `thread.name(...)` stability `stable`
- `proc stack_size` signature `proc stack_size(builder: ref mut ThreadBuilder, value: usize) { set (*builder).stack_size = some<usize>(value); }` example `thread.stack_size(...)` stability `stable`
- `proc spawn_with` signature `proc spawn_with<T>(builder: ThreadBuilder, f: proc() -> T) -> Result<JoinHandle<T>, ThreadError> { give compiler_backend_thread_spawn_with<T>(builder, f); }` example `thread.spawn_with(...)` stability `stable`
- `proc join` signature `proc join<T>(handle: JoinHandle<T>) -> Result<T, ThreadError> { give compiler_backend_thread_join<T>(handle); }` example `thread.join(...)` stability `stable`
- `proc sleep` signature `proc sleep(duration: Duration) { compiler_backend_thread_sleep(duration); }` example `thread.sleep(...)` stability `stable`
- `proc yield_now` signature `proc yield_now() { compiler_backend_thread_yield_now(); }` example `thread.yield_now(...)` stability `stable`

## `src/vitte/stdlib/std/time.vitl`

Stability: `stable`

- `form Duration` signature `form Duration { nanos: u128 }` example `time.Duration` stability `stable`
- `form Instant` signature `form Instant { ticks: u128 }` example `time.Instant` stability `stable`
- `form SystemTime` signature `form SystemTime { nanos_since_epoch: i128 }` example `time.SystemTime` stability `stable`
- `form DateTime` signature `form DateTime { year: i32 month: u8 day: u8 hour: u8 minute: u8 second: u8 nanos: u32 offset_seconds: i32 }` example `time.DateTime` stability `stable`
- `proc duration_from_secs` signature `proc duration_from_secs(secs: u64) -> Duration { give Duration { nanos: secs as u128 * 1000000000 }; }` example `time.duration_from_secs(...)` stability `stable`
- `proc duration_from_millis` signature `proc duration_from_millis(ms: u64) -> Duration { give Duration { nanos: ms as u128 * 1000000 }; }` example `time.duration_from_millis(...)` stability `stable`
- `proc duration_from_micros` signature `proc duration_from_micros(us: u64) -> Duration { give Duration { nanos: us as u128 * 1000 }; }` example `time.duration_from_micros(...)` stability `stable`
- `proc duration_from_nanos` signature `proc duration_from_nanos(ns: u128) -> Duration { give Duration { nanos: ns }; }` example `time.duration_from_nanos(...)` stability `stable`
- `proc duration_as_nanos` signature `proc duration_as_nanos(value: Duration) -> u128 { give value.nanos; }` example `time.duration_as_nanos(...)` stability `stable`
- `proc duration_as_secs` signature `proc duration_as_secs(value: Duration) -> u64 { give (value.nanos / 1000000000) as u64; }` example `time.duration_as_secs(...)` stability `stable`
- `proc duration_checked_add` signature `proc duration_checked_add(left: Duration, right: Duration) -> Option<Duration> {` example `time.duration_checked_add(...)` stability `stable`
- `proc duration_checked_sub` signature `proc duration_checked_sub(left: Duration, right: Duration) -> Option<Duration> {` example `time.duration_checked_sub(...)` stability `stable`
- `proc instant_now` signature `proc instant_now() -> Instant { give compiler_backend_instant_now(); }` example `time.instant_now(...)` stability `stable`
- `proc system_time_now` signature `proc system_time_now() -> SystemTime { give compiler_backend_system_time_now(); }` example `time.system_time_now(...)` stability `stable`
- `proc elapsed` signature `proc elapsed(start: Instant) -> Duration {` example `time.elapsed(...)` stability `stable`
- `proc checked_add_duration` signature `proc checked_add_duration(time: Instant, duration: Duration) -> Option<Instant> {` example `time.checked_add_duration(...)` stability `stable`
- `proc checked_sub_duration` signature `proc checked_sub_duration(time: Instant, duration: Duration) -> Option<Instant> {` example `time.checked_sub_duration(...)` stability `stable`
- `proc system_time_to_datetime` signature `proc system_time_to_datetime(time: SystemTime) -> DateTime { give compiler_backend_time_to_datetime(time); }` example `time.system_time_to_datetime(...)` stability `stable`
- `proc datetime_to_system_time` signature `proc datetime_to_system_time(value: DateTime) -> SystemTime { give compiler_backend_datetime_to_system_time(value); }` example `time.datetime_to_system_time(...)` stability `stable`
- `proc format_datetime` signature `proc format_datetime(value: DateTime, pattern: string) -> String { give compiler_backend_format_datetime(value, pattern); }` example `time.format_datetime(...)` stability `stable`
- `proc format_duration` signature `proc format_duration(value: Duration) -> String { give compiler_backend_format_duration(value); }` example `time.format_duration(...)` stability `stable`
- `proc checked_add_system_time` signature `proc checked_add_system_time(time: SystemTime, duration: Duration) -> Option<SystemTime> {` example `time.checked_add_system_time(...)` stability `stable`

## `src/vitte/stdlib/std/terminal.vitl`

Stability: `stable`

- `pick TerminalColor` signature `pick TerminalColor { Default Black Red Green Yellow Blue Magenta Cyan White }` example `terminal.TerminalColor` stability `stable`
- `form TerminalStyle` signature `form TerminalStyle { fg: TerminalColor bg: TerminalColor bold: bool underline: bool }` example `terminal.TerminalStyle` stability `stable`
- `form TerminalSize` signature `form TerminalSize { columns: u16 rows: u16 }` example `terminal.TerminalSize` stability `stable`
- `proc terminal_size` signature `proc terminal_size() -> Option<TerminalSize> { give compiler_terminal_size(); }` example `terminal.terminal_size(...)` stability `stable`
- `proc terminal_is_tty` signature `proc terminal_is_tty(writer: Writer) -> bool { give compiler_terminal_is_tty(writer); }` example `terminal.terminal_is_tty(...)` stability `stable`
- `proc terminal_style` signature `proc terminal_style() -> TerminalStyle { give TerminalStyle { fg: TerminalColor.Default, bg: TerminalColor.Default, bold: false, underline: false }; }` example `terminal.terminal_style(...)` stability `stable`
- `proc terminal_apply` signature `proc terminal_apply(text: String, style: TerminalStyle) -> String { give compiler_terminal_apply(text, style); }` example `terminal.terminal_apply(...)` stability `stable`
- `proc terminal_clear` signature `proc terminal_clear(writer: Writer) -> Result<(), IoError> { give compiler_terminal_clear(writer); }` example `terminal.terminal_clear(...)` stability `stable`

## `src/vitte/stdlib/std/units.vitl`

Stability: `stable`

- `pick UnitKind` signature `pick UnitKind { Length Mass Time Data Temperature }` example `units.UnitKind` stability `stable`
- `form Unit` signature `form Unit { name: String symbol: String kind: UnitKind scale: f64 }` example `units.Unit` stability `stable`
- `form Quantity` signature `form Quantity { value: f64 unit: Unit }` example `units.Quantity` stability `stable`
- `proc unit` signature `proc unit(name: String, symbol: String, kind: UnitKind, scale: f64) -> Unit { give Unit { name: name, symbol: symbol, kind: kind, scale: scale }; }` example `units.unit(...)` stability `stable`
- `proc quantity` signature `proc quantity(value: f64, unit_value: Unit) -> Quantity { give Quantity { value: value, unit: unit_value }; }` example `units.quantity(...)` stability `stable`
- `proc convert` signature `proc convert(value: Quantity, target: Unit) -> Quantity {` example `units.convert(...)` stability `stable`
- `proc meter` signature `proc meter() -> Unit { give unit(compiler_test_string("meter"), compiler_test_string("m"), UnitKind.Length, 1.0); }` example `units.meter(...)` stability `stable`
- `proc byte_unit` signature `proc byte_unit() -> Unit { give unit(compiler_test_string("byte"), compiler_test_string("B"), UnitKind.Data, 1.0); }` example `units.byte_unit(...)` stability `stable`

## `src/vitte/stdlib/std/uri.vitl`

Stability: `stable`

- `form Uri` signature `form Uri {` example `uri.Uri` stability `stable`
- `form UriError` signature `form UriError {` example `uri.UriError` stability `stable`
- `proc uri_parse` signature `proc uri_parse(text: Utf8View) -> Result<Uri, UriError> { give compiler_uri_parse(text); }` example `uri.uri_parse(...)` stability `stable`
- `proc uri_to_string` signature `proc uri_to_string(value: Uri) -> String { give compiler_uri_to_string(value); }` example `uri.uri_to_string(...)` stability `stable`
- `proc uri_path` signature `proc uri_path(value: Uri) -> String { give value.path; }` example `uri.uri_path(...)` stability `stable`
- `proc uri_query` signature `proc uri_query(value: Uri) -> Option<String> { give value.query; }` example `uri.uri_query(...)` stability `stable`

## `src/vitte/stdlib/std/url.vitl`

Stability: `stable`

- `form Url` signature `form Url {` example `url.Url` stability `stable`
- `form UrlError` signature `form UrlError {` example `url.UrlError` stability `stable`
- `proc url_parse` signature `proc url_parse(text: Utf8View) -> Result<Url, UrlError> { give compiler_url_parse(text); }` example `url.url_parse(...)` stability `stable`
- `proc url_to_string` signature `proc url_to_string(value: Url) -> String { give compiler_url_to_string(value); }` example `url.url_to_string(...)` stability `stable`
- `proc url_join` signature `proc url_join(base: Url, reference: Utf8View) -> Result<Url, UrlError> { give compiler_url_join(base, reference); }` example `url.url_join(...)` stability `stable`
- `proc url_scheme` signature `proc url_scheme(value: Url) -> String { give value.scheme; }` example `url.url_scheme(...)` stability `stable`
- `proc url_path` signature `proc url_path(value: Url) -> String { give value.path; }` example `url.url_path(...)` stability `stable`

## `src/vitte/stdlib/std/uuid.vitl`

Stability: `stable`

- `form Uuid` signature `form Uuid {` example `uuid.Uuid` stability `stable`
- `form UuidError` signature `form UuidError {` example `uuid.UuidError` stability `stable`
- `proc uuid_nil` signature `proc uuid_nil() -> Uuid {` example `uuid.uuid_nil(...)` stability `stable`
- `proc uuid_error` signature `proc uuid_error(message: String) -> UuidError { give UuidError { message: message }; }` example `uuid.uuid_error(...)` stability `stable`
- `proc uuid_v4` signature `proc uuid_v4() -> Result<Uuid, UuidError> {` example `uuid.uuid_v4(...)` stability `stable`
- `proc uuid_parse` signature `proc uuid_parse(text: Utf8View) -> Result<Uuid, UuidError> {` example `uuid.uuid_parse(...)` stability `stable`
- `proc uuid_to_string` signature `proc uuid_to_string(value: Uuid) -> String {` example `uuid.uuid_to_string(...)` stability `stable`
- `proc uuid_is_nil` signature `proc uuid_is_nil(value: Uuid) -> bool {` example `uuid.uuid_is_nil(...)` stability `stable`

## `src/vitte/stdlib/tests/api_contracts.vit`

Stability: `internal`

- `proc stdlib_api_contracts_smoke` signature `proc stdlib_api_contracts_smoke() -> bool {` example `api_contracts.stdlib_api_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/alloc_memory_invariants.vit`

Stability: `internal`

- `proc alloc_vec_memory_invariants` signature `proc alloc_vec_memory_invariants() -> bool {` example `alloc_memory_invariants.alloc_vec_memory_invariants(...)` stability `internal`
- `proc alloc_string_memory_invariants` signature `proc alloc_string_memory_invariants() -> bool {` example `alloc_memory_invariants.alloc_string_memory_invariants(...)` stability `internal`
- `proc alloc_collections_memory_invariants` signature `proc alloc_collections_memory_invariants() -> bool {` example `alloc_memory_invariants.alloc_collections_memory_invariants(...)` stability `internal`
- `proc alloc_memory_invariants_run` signature `proc alloc_memory_invariants_run() -> bool {` example `alloc_memory_invariants.alloc_memory_invariants_run(...)` stability `internal`

## `src/vitte/stdlib/tests/core_alloc_contracts.vit`

Stability: `internal`

- `proc stdlib_core_alloc_contracts_smoke` signature `proc stdlib_core_alloc_contracts_smoke() -> bool {` example `core_alloc_contracts.stdlib_core_alloc_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/path_json_parse.vit`

Stability: `internal`

- `proc fuzz_path_json_parse` signature `proc fuzz_path_json_parse(seed: u64) -> bool {` example `path_json_parse.fuzz_path_json_parse(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/path_fuzz.vit`

Stability: `internal`

- `proc fuzz_path` signature `proc fuzz_path(seed: u64) -> bool {` example `path_fuzz.fuzz_path(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/minimize.vit`

Stability: `internal`

- `form FuzzFailure` signature `form FuzzFailure { seed: u64 input: [byte] minimized: [byte] }` example `minimize.FuzzFailure` stability `internal`
- `proc minimize_failure` signature `proc minimize_failure(seed: u64, input: [byte], predicate: proc([byte]) -> bool) -> FuzzFailure {` example `minimize.minimize_failure(...)` stability `internal`
- `proc fuzz_minimizer_smoke` signature `proc fuzz_minimizer_smoke(seed: u64) -> bool {` example `minimize.fuzz_minimizer_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/platform/posix_test.vit`

Stability: `internal`

- `proc platform_posix_test` signature `proc platform_posix_test() -> bool {` example `posix_test.platform_posix_test(...)` stability `internal`

## `src/vitte/stdlib/tests/platform/windows_test.vit`

Stability: `internal`

- `proc platform_windows_test` signature `proc platform_windows_test() -> bool {` example `windows_test.platform_windows_test(...)` stability `internal`

## `src/vitte/stdlib/tests/platform/wasm_test.vit`

Stability: `internal`

- `proc platform_wasm_test` signature `proc platform_wasm_test() -> bool {` example `wasm_test.platform_wasm_test(...)` stability `internal`

## `src/vitte/stdlib/tests/platform/embedded_test.vit`

Stability: `internal`

- `proc platform_embedded_test` signature `proc platform_embedded_test() -> bool {` example `embedded_test.platform_embedded_test(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/json_fuzz.vit`

Stability: `internal`

- `proc fuzz_json` signature `proc fuzz_json(seed: u64) -> bool {` example `json_fuzz.fuzz_json(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/csv_fuzz.vit`

Stability: `internal`

- `proc fuzz_csv` signature `proc fuzz_csv(seed: u64) -> bool {` example `csv_fuzz.fuzz_csv(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/url_fuzz.vit`

Stability: `internal`

- `proc fuzz_url` signature `proc fuzz_url(seed: u64) -> bool {` example `url_fuzz.fuzz_url(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/utf8_fuzz.vit`

Stability: `internal`

- `proc fuzz_utf8` signature `proc fuzz_utf8(seed: u64) -> bool {` example `utf8_fuzz.fuzz_utf8(...)` stability `internal`

## `src/vitte/stdlib/tests/fuzz/utf8_url_csv.vit`

Stability: `internal`

- `proc fuzz_utf8_url_csv` signature `proc fuzz_utf8_url_csv(seed: u64) -> bool {` example `utf8_url_csv.fuzz_utf8_url_csv(...)` stability `internal`

## `src/vitte/stdlib/tests/module_runner.vit`

Stability: `internal`

- `proc stdlib_module_tests_run` signature `proc stdlib_module_tests_run() -> bool {` example `module_runner.stdlib_module_tests_run(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/alloc_api_test.vit`

Stability: `internal`

- `proc alloc_api_test` signature `proc alloc_api_test() -> bool {` example `alloc_api_test.alloc_api_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_io_process_test.vit`

Stability: `internal`

- `proc std_io_process_test` signature `proc std_io_process_test() -> bool {` example `std_io_process_test.std_io_process_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_thread_sync_time_env_test.vit`

Stability: `internal`

- `proc std_thread_sync_time_env_test` signature `proc std_thread_sync_time_env_test() -> bool {` example `std_thread_sync_time_env_test.std_thread_sync_time_env_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_calendar_test.vit`

Stability: `internal`

- `proc std_calendar_test` signature `proc std_calendar_test() -> bool {` example `std_calendar_test.std_calendar_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_metrics_test.vit`

Stability: `internal`

- `proc std_metrics_test` signature `proc std_metrics_test() -> bool {` example `std_metrics_test.std_metrics_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_mime_test.vit`

Stability: `internal`

- `proc std_mime_test` signature `proc std_mime_test() -> bool {` example `std_mime_test.std_mime_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_percent_encoding_test.vit`

Stability: `internal`

- `proc std_percent_encoding_test` signature `proc std_percent_encoding_test() -> bool {` example `std_percent_encoding_test.std_percent_encoding_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_units_test.vit`

Stability: `internal`

- `proc std_units_test` signature `proc std_units_test() -> bool {` example `std_units_test.std_units_test(...)` stability `internal`

## `src/vitte/stdlib/tests/modules/std_uuid_test.vit`

Stability: `internal`

- `proc std_uuid_test` signature `proc std_uuid_test() -> bool {` example `std_uuid_test.std_uuid_test(...)` stability `internal`

## `src/vitte/stdlib/tests/negative/stdlib_negative_cases.vit`

Stability: `internal`

- `proc stdlib_negative_invalid_inputs` signature `proc stdlib_negative_invalid_inputs() -> bool {` example `stdlib_negative_cases.stdlib_negative_invalid_inputs(...)` stability `internal`
- `proc stdlib_negative_boundaries` signature `proc stdlib_negative_boundaries() -> bool {` example `stdlib_negative_cases.stdlib_negative_boundaries(...)` stability `internal`
- `proc stdlib_negative_overflow` signature `proc stdlib_negative_overflow() -> bool {` example `stdlib_negative_cases.stdlib_negative_overflow(...)` stability `internal`
- `proc stdlib_negative_cases_run` signature `proc stdlib_negative_cases_run() -> bool {` example `stdlib_negative_cases.stdlib_negative_cases_run(...)` stability `internal`

## `src/vitte/stdlib/tests/public_module_coverage.vit`

Stability: `internal`

- `const PUBLIC_MODULE_TEST_COUNT` signature `const PUBLIC_MODULE_TEST_COUNT: usize = 153` example `public_module_coverage.PUBLIC_MODULE_TEST_COUNT` stability `internal`
- `const PUBLIC_MODULE_000_TEST` signature `const PUBLIC_MODULE_000_TEST: string = "src/vitte/stdlib/mod.vit"` example `public_module_coverage.PUBLIC_MODULE_000_TEST` stability `internal`
- `const PUBLIC_MODULE_001_TEST` signature `const PUBLIC_MODULE_001_TEST: string = "src/vitte/stdlib/core.vitl"` example `public_module_coverage.PUBLIC_MODULE_001_TEST` stability `internal`
- `const PUBLIC_MODULE_002_TEST` signature `const PUBLIC_MODULE_002_TEST: string = "src/vitte/stdlib/core_alias.vitl"` example `public_module_coverage.PUBLIC_MODULE_002_TEST` stability `internal`
- `const PUBLIC_MODULE_003_TEST` signature `const PUBLIC_MODULE_003_TEST: string = "src/vitte/stdlib/memory.vitl"` example `public_module_coverage.PUBLIC_MODULE_003_TEST` stability `internal`
- `const PUBLIC_MODULE_004_TEST` signature `const PUBLIC_MODULE_004_TEST: string = "src/vitte/stdlib/collections.vitl"` example `public_module_coverage.PUBLIC_MODULE_004_TEST` stability `internal`
- `const PUBLIC_MODULE_005_TEST` signature `const PUBLIC_MODULE_005_TEST: string = "src/vitte/stdlib/math.vitl"` example `public_module_coverage.PUBLIC_MODULE_005_TEST` stability `internal`
- `const PUBLIC_MODULE_006_TEST` signature `const PUBLIC_MODULE_006_TEST: string = "src/vitte/stdlib/strings.vitl"` example `public_module_coverage.PUBLIC_MODULE_006_TEST` stability `internal`
- `const PUBLIC_MODULE_007_TEST` signature `const PUBLIC_MODULE_007_TEST: string = "src/vitte/stdlib/encoding.vitl"` example `public_module_coverage.PUBLIC_MODULE_007_TEST` stability `internal`
- `const PUBLIC_MODULE_008_TEST` signature `const PUBLIC_MODULE_008_TEST: string = "src/vitte/stdlib/json.vitl"` example `public_module_coverage.PUBLIC_MODULE_008_TEST` stability `internal`
- `const PUBLIC_MODULE_009_TEST` signature `const PUBLIC_MODULE_009_TEST: string = "src/vitte/stdlib/crypto.vitl"` example `public_module_coverage.PUBLIC_MODULE_009_TEST` stability `internal`
- `const PUBLIC_MODULE_010_TEST` signature `const PUBLIC_MODULE_010_TEST: string = "src/vitte/stdlib/compression.vitl"` example `public_module_coverage.PUBLIC_MODULE_010_TEST` stability `internal`
- `const PUBLIC_MODULE_011_TEST` signature `const PUBLIC_MODULE_011_TEST: string = "src/vitte/stdlib/regex.vitl"` example `public_module_coverage.PUBLIC_MODULE_011_TEST` stability `internal`
- `const PUBLIC_MODULE_012_TEST` signature `const PUBLIC_MODULE_012_TEST: string = "src/vitte/stdlib/io.vitl"` example `public_module_coverage.PUBLIC_MODULE_012_TEST` stability `internal`
- `const PUBLIC_MODULE_013_TEST` signature `const PUBLIC_MODULE_013_TEST: string = "src/vitte/stdlib/path.vitl"` example `public_module_coverage.PUBLIC_MODULE_013_TEST` stability `internal`
- `const PUBLIC_MODULE_014_TEST` signature `const PUBLIC_MODULE_014_TEST: string = "src/vitte/stdlib/os.vitl"` example `public_module_coverage.PUBLIC_MODULE_014_TEST` stability `internal`
- `const PUBLIC_MODULE_015_TEST` signature `const PUBLIC_MODULE_015_TEST: string = "src/vitte/stdlib/sysinfo.vitl"` example `public_module_coverage.PUBLIC_MODULE_015_TEST` stability `internal`
- `const PUBLIC_MODULE_016_TEST` signature `const PUBLIC_MODULE_016_TEST: string = "src/vitte/stdlib/datetime.vitl"` example `public_module_coverage.PUBLIC_MODULE_016_TEST` stability `internal`
- `const PUBLIC_MODULE_017_TEST` signature `const PUBLIC_MODULE_017_TEST: string = "src/vitte/stdlib/runtime.vitl"` example `public_module_coverage.PUBLIC_MODULE_017_TEST` stability `internal`
- `const PUBLIC_MODULE_018_TEST` signature `const PUBLIC_MODULE_018_TEST: string = "src/vitte/stdlib/kernel.vitl"` example `public_module_coverage.PUBLIC_MODULE_018_TEST` stability `internal`
- `const PUBLIC_MODULE_019_TEST` signature `const PUBLIC_MODULE_019_TEST: string = "src/vitte/stdlib/graphics.vitl"` example `public_module_coverage.PUBLIC_MODULE_019_TEST` stability `internal`
- `const PUBLIC_MODULE_020_TEST` signature `const PUBLIC_MODULE_020_TEST: string = "src/vitte/stdlib/core/algorithms.vitl"` example `public_module_coverage.PUBLIC_MODULE_020_TEST` stability `internal`
- `const PUBLIC_MODULE_021_TEST` signature `const PUBLIC_MODULE_021_TEST: string = "src/vitte/stdlib/core/array.vitl"` example `public_module_coverage.PUBLIC_MODULE_021_TEST` stability `internal`
- `const PUBLIC_MODULE_022_TEST` signature `const PUBLIC_MODULE_022_TEST: string = "src/vitte/stdlib/core/ascii.vitl"` example `public_module_coverage.PUBLIC_MODULE_022_TEST` stability `internal`
- `const PUBLIC_MODULE_023_TEST` signature `const PUBLIC_MODULE_023_TEST: string = "src/vitte/stdlib/core/clone.vitl"` example `public_module_coverage.PUBLIC_MODULE_023_TEST` stability `internal`
- `const PUBLIC_MODULE_024_TEST` signature `const PUBLIC_MODULE_024_TEST: string = "src/vitte/stdlib/core/cmp.vitl"` example `public_module_coverage.PUBLIC_MODULE_024_TEST` stability `internal`
- `const PUBLIC_MODULE_025_TEST` signature `const PUBLIC_MODULE_025_TEST: string = "src/vitte/stdlib/core/concurrency.vitl"` example `public_module_coverage.PUBLIC_MODULE_025_TEST` stability `internal`
- `const PUBLIC_MODULE_026_TEST` signature `const PUBLIC_MODULE_026_TEST: string = "src/vitte/stdlib/core/convert.vitl"` example `public_module_coverage.PUBLIC_MODULE_026_TEST` stability `internal`
- `const PUBLIC_MODULE_027_TEST` signature `const PUBLIC_MODULE_027_TEST: string = "src/vitte/stdlib/core/default.vitl"` example `public_module_coverage.PUBLIC_MODULE_027_TEST` stability `internal`
- `const PUBLIC_MODULE_028_TEST` signature `const PUBLIC_MODULE_028_TEST: string = "src/vitte/stdlib/core/drop.vitl"` example `public_module_coverage.PUBLIC_MODULE_028_TEST` stability `internal`
- `const PUBLIC_MODULE_029_TEST` signature `const PUBLIC_MODULE_029_TEST: string = "src/vitte/stdlib/core/float.vitl"` example `public_module_coverage.PUBLIC_MODULE_029_TEST` stability `internal`
- `const PUBLIC_MODULE_030_TEST` signature `const PUBLIC_MODULE_030_TEST: string = "src/vitte/stdlib/core/hash.vitl"` example `public_module_coverage.PUBLIC_MODULE_030_TEST` stability `internal`
- `const PUBLIC_MODULE_031_TEST` signature `const PUBLIC_MODULE_031_TEST: string = "src/vitte/stdlib/core/io_helpers.vitl"` example `public_module_coverage.PUBLIC_MODULE_031_TEST` stability `internal`
- `const PUBLIC_MODULE_032_TEST` signature `const PUBLIC_MODULE_032_TEST: string = "src/vitte/stdlib/core/iterator.vitl"` example `public_module_coverage.PUBLIC_MODULE_032_TEST` stability `internal`
- `const PUBLIC_MODULE_033_TEST` signature `const PUBLIC_MODULE_033_TEST: string = "src/vitte/stdlib/core/math.vitl"` example `public_module_coverage.PUBLIC_MODULE_033_TEST` stability `internal`
- `const PUBLIC_MODULE_034_TEST` signature `const PUBLIC_MODULE_034_TEST: string = "src/vitte/stdlib/core/memory.vitl"` example `public_module_coverage.PUBLIC_MODULE_034_TEST` stability `internal`
- `const PUBLIC_MODULE_035_TEST` signature `const PUBLIC_MODULE_035_TEST: string = "src/vitte/stdlib/core/number.vitl"` example `public_module_coverage.PUBLIC_MODULE_035_TEST` stability `internal`
- `const PUBLIC_MODULE_036_TEST` signature `const PUBLIC_MODULE_036_TEST: string = "src/vitte/stdlib/core/panic.vitl"` example `public_module_coverage.PUBLIC_MODULE_036_TEST` stability `internal`
- `const PUBLIC_MODULE_037_TEST` signature `const PUBLIC_MODULE_037_TEST: string = "src/vitte/stdlib/core/option.vitl"` example `public_module_coverage.PUBLIC_MODULE_037_TEST` stability `internal`
- `const PUBLIC_MODULE_038_TEST` signature `const PUBLIC_MODULE_038_TEST: string = "src/vitte/stdlib/core/primitive.vitl"` example `public_module_coverage.PUBLIC_MODULE_038_TEST` stability `internal`
- `const PUBLIC_MODULE_039_TEST` signature `const PUBLIC_MODULE_039_TEST: string = "src/vitte/stdlib/core/range.vitl"` example `public_module_coverage.PUBLIC_MODULE_039_TEST` stability `internal`
- `const PUBLIC_MODULE_040_TEST` signature `const PUBLIC_MODULE_040_TEST: string = "src/vitte/stdlib/core/result.vitl"` example `public_module_coverage.PUBLIC_MODULE_040_TEST` stability `internal`
- `const PUBLIC_MODULE_041_TEST` signature `const PUBLIC_MODULE_041_TEST: string = "src/vitte/stdlib/core/scope.vitl"` example `public_module_coverage.PUBLIC_MODULE_041_TEST` stability `internal`
- `const PUBLIC_MODULE_042_TEST` signature `const PUBLIC_MODULE_042_TEST: string = "src/vitte/stdlib/core/slice.vitl"` example `public_module_coverage.PUBLIC_MODULE_042_TEST` stability `internal`
- `const PUBLIC_MODULE_043_TEST` signature `const PUBLIC_MODULE_043_TEST: string = "src/vitte/stdlib/core/string.vitl"` example `public_module_coverage.PUBLIC_MODULE_043_TEST` stability `internal`
- `const PUBLIC_MODULE_044_TEST` signature `const PUBLIC_MODULE_044_TEST: string = "src/vitte/stdlib/core/strings.vitl"` example `public_module_coverage.PUBLIC_MODULE_044_TEST` stability `internal`
- `const PUBLIC_MODULE_045_TEST` signature `const PUBLIC_MODULE_045_TEST: string = "src/vitte/stdlib/core/types.vitl"` example `public_module_coverage.PUBLIC_MODULE_045_TEST` stability `internal`
- `const PUBLIC_MODULE_046_TEST` signature `const PUBLIC_MODULE_046_TEST: string = "src/vitte/stdlib/core/unicode.vitl"` example `public_module_coverage.PUBLIC_MODULE_046_TEST` stability `internal`
- `const PUBLIC_MODULE_047_TEST` signature `const PUBLIC_MODULE_047_TEST: string = "src/vitte/stdlib/core/utils.vitl"` example `public_module_coverage.PUBLIC_MODULE_047_TEST` stability `internal`
- `const PUBLIC_MODULE_048_TEST` signature `const PUBLIC_MODULE_048_TEST: string = "src/vitte/stdlib/alloc/arc.vitl"` example `public_module_coverage.PUBLIC_MODULE_048_TEST` stability `internal`
- `const PUBLIC_MODULE_049_TEST` signature `const PUBLIC_MODULE_049_TEST: string = "src/vitte/stdlib/alloc/box.vitl"` example `public_module_coverage.PUBLIC_MODULE_049_TEST` stability `internal`
- `const PUBLIC_MODULE_050_TEST` signature `const PUBLIC_MODULE_050_TEST: string = "src/vitte/stdlib/alloc/collections.vitl"` example `public_module_coverage.PUBLIC_MODULE_050_TEST` stability `internal`
- `const PUBLIC_MODULE_051_TEST` signature `const PUBLIC_MODULE_051_TEST: string = "src/vitte/stdlib/alloc/rc.vitl"` example `public_module_coverage.PUBLIC_MODULE_051_TEST` stability `internal`
- `const PUBLIC_MODULE_052_TEST` signature `const PUBLIC_MODULE_052_TEST: string = "src/vitte/stdlib/alloc/string.vitl"` example `public_module_coverage.PUBLIC_MODULE_052_TEST` stability `internal`
- `const PUBLIC_MODULE_053_TEST` signature `const PUBLIC_MODULE_053_TEST: string = "src/vitte/stdlib/alloc/vec.vitl"` example `public_module_coverage.PUBLIC_MODULE_053_TEST` stability `internal`
- `const PUBLIC_MODULE_054_TEST` signature `const PUBLIC_MODULE_054_TEST: string = "src/vitte/stdlib/async/async.vitl"` example `public_module_coverage.PUBLIC_MODULE_054_TEST` stability `internal`
- `const PUBLIC_MODULE_055_TEST` signature `const PUBLIC_MODULE_055_TEST: string = "src/vitte/stdlib/async/channel.vitl"` example `public_module_coverage.PUBLIC_MODULE_055_TEST` stability `internal`
- `const PUBLIC_MODULE_056_TEST` signature `const PUBLIC_MODULE_056_TEST: string = "src/vitte/stdlib/async/future.vitl"` example `public_module_coverage.PUBLIC_MODULE_056_TEST` stability `internal`
- `const PUBLIC_MODULE_057_TEST` signature `const PUBLIC_MODULE_057_TEST: string = "src/vitte/stdlib/collections/queue.vitl"` example `public_module_coverage.PUBLIC_MODULE_057_TEST` stability `internal`
- `const PUBLIC_MODULE_058_TEST` signature `const PUBLIC_MODULE_058_TEST: string = "src/vitte/stdlib/collections/vector.vitl"` example `public_module_coverage.PUBLIC_MODULE_058_TEST` stability `internal`
- `const PUBLIC_MODULE_059_TEST` signature `const PUBLIC_MODULE_059_TEST: string = "src/vitte/stdlib/compression/algorithms.vitl"` example `public_module_coverage.PUBLIC_MODULE_059_TEST` stability `internal`
- `const PUBLIC_MODULE_060_TEST` signature `const PUBLIC_MODULE_060_TEST: string = "src/vitte/stdlib/compression/brotli.vitl"` example `public_module_coverage.PUBLIC_MODULE_060_TEST` stability `internal`
- `const PUBLIC_MODULE_061_TEST` signature `const PUBLIC_MODULE_061_TEST: string = "src/vitte/stdlib/compression/deflate.vitl"` example `public_module_coverage.PUBLIC_MODULE_061_TEST` stability `internal`
- `const PUBLIC_MODULE_062_TEST` signature `const PUBLIC_MODULE_062_TEST: string = "src/vitte/stdlib/compression/huffman.vitl"` example `public_module_coverage.PUBLIC_MODULE_062_TEST` stability `internal`
- `const PUBLIC_MODULE_063_TEST` signature `const PUBLIC_MODULE_063_TEST: string = "src/vitte/stdlib/compression/interface.vitl"` example `public_module_coverage.PUBLIC_MODULE_063_TEST` stability `internal`
- `const PUBLIC_MODULE_064_TEST` signature `const PUBLIC_MODULE_064_TEST: string = "src/vitte/stdlib/compression/lz.vitl"` example `public_module_coverage.PUBLIC_MODULE_064_TEST` stability `internal`
- `const PUBLIC_MODULE_065_TEST` signature `const PUBLIC_MODULE_065_TEST: string = "src/vitte/stdlib/compression/stats.vitl"` example `public_module_coverage.PUBLIC_MODULE_065_TEST` stability `internal`
- `const PUBLIC_MODULE_066_TEST` signature `const PUBLIC_MODULE_066_TEST: string = "src/vitte/stdlib/crypto/hash.vitl"` example `public_module_coverage.PUBLIC_MODULE_066_TEST` stability `internal`
- `const PUBLIC_MODULE_067_TEST` signature `const PUBLIC_MODULE_067_TEST: string = "src/vitte/stdlib/crypto/keyderivation.vitl"` example `public_module_coverage.PUBLIC_MODULE_067_TEST` stability `internal`
- `const PUBLIC_MODULE_068_TEST` signature `const PUBLIC_MODULE_068_TEST: string = "src/vitte/stdlib/crypto/utils.vitl"` example `public_module_coverage.PUBLIC_MODULE_068_TEST` stability `internal`
- `const PUBLIC_MODULE_069_TEST` signature `const PUBLIC_MODULE_069_TEST: string = "src/vitte/stdlib/encoding/utf.vitl"` example `public_module_coverage.PUBLIC_MODULE_069_TEST` stability `internal`
- `const PUBLIC_MODULE_070_TEST` signature `const PUBLIC_MODULE_070_TEST: string = "src/vitte/stdlib/json/builder.vitl"` example `public_module_coverage.PUBLIC_MODULE_070_TEST` stability `internal`
- `const PUBLIC_MODULE_071_TEST` signature `const PUBLIC_MODULE_071_TEST: string = "src/vitte/stdlib/json/parse.vitl"` example `public_module_coverage.PUBLIC_MODULE_071_TEST` stability `internal`
- `const PUBLIC_MODULE_072_TEST` signature `const PUBLIC_MODULE_072_TEST: string = "src/vitte/stdlib/json/stringify.vitl"` example `public_module_coverage.PUBLIC_MODULE_072_TEST` stability `internal`
- `const PUBLIC_MODULE_073_TEST` signature `const PUBLIC_MODULE_073_TEST: string = "src/vitte/stdlib/json/types.vitl"` example `public_module_coverage.PUBLIC_MODULE_073_TEST` stability `internal`
- `const PUBLIC_MODULE_074_TEST` signature `const PUBLIC_MODULE_074_TEST: string = "src/vitte/stdlib/kernel/fileio.vitl"` example `public_module_coverage.PUBLIC_MODULE_074_TEST` stability `internal`
- `const PUBLIC_MODULE_075_TEST` signature `const PUBLIC_MODULE_075_TEST: string = "src/vitte/stdlib/kernel/memory.vitl"` example `public_module_coverage.PUBLIC_MODULE_075_TEST` stability `internal`
- `const PUBLIC_MODULE_076_TEST` signature `const PUBLIC_MODULE_076_TEST: string = "src/vitte/stdlib/kernel/sync.vitl"` example `public_module_coverage.PUBLIC_MODULE_076_TEST` stability `internal`
- `const PUBLIC_MODULE_077_TEST` signature `const PUBLIC_MODULE_077_TEST: string = "src/vitte/stdlib/kernel/users.vitl"` example `public_module_coverage.PUBLIC_MODULE_077_TEST` stability `internal`
- `const PUBLIC_MODULE_078_TEST` signature `const PUBLIC_MODULE_078_TEST: string = "src/vitte/stdlib/math/algebra.vitl"` example `public_module_coverage.PUBLIC_MODULE_078_TEST` stability `internal`
- `const PUBLIC_MODULE_079_TEST` signature `const PUBLIC_MODULE_079_TEST: string = "src/vitte/stdlib/math/arithmetic.vitl"` example `public_module_coverage.PUBLIC_MODULE_079_TEST` stability `internal`
- `const PUBLIC_MODULE_080_TEST` signature `const PUBLIC_MODULE_080_TEST: string = "src/vitte/stdlib/math/arrays.vitl"` example `public_module_coverage.PUBLIC_MODULE_080_TEST` stability `internal`
- `const PUBLIC_MODULE_081_TEST` signature `const PUBLIC_MODULE_081_TEST: string = "src/vitte/stdlib/math/calculus.vitl"` example `public_module_coverage.PUBLIC_MODULE_081_TEST` stability `internal`
- `const PUBLIC_MODULE_082_TEST` signature `const PUBLIC_MODULE_082_TEST: string = "src/vitte/stdlib/math/comparison.vitl"` example `public_module_coverage.PUBLIC_MODULE_082_TEST` stability `internal`
- `const PUBLIC_MODULE_083_TEST` signature `const PUBLIC_MODULE_083_TEST: string = "src/vitte/stdlib/math/complex.vitl"` example `public_module_coverage.PUBLIC_MODULE_083_TEST` stability `internal`
- `const PUBLIC_MODULE_084_TEST` signature `const PUBLIC_MODULE_084_TEST: string = "src/vitte/stdlib/math/geometry.vitl"` example `public_module_coverage.PUBLIC_MODULE_084_TEST` stability `internal`
- `const PUBLIC_MODULE_085_TEST` signature `const PUBLIC_MODULE_085_TEST: string = "src/vitte/stdlib/math/matrix.vitl"` example `public_module_coverage.PUBLIC_MODULE_085_TEST` stability `internal`
- `const PUBLIC_MODULE_086_TEST` signature `const PUBLIC_MODULE_086_TEST: string = "src/vitte/stdlib/math/modular.vitl"` example `public_module_coverage.PUBLIC_MODULE_086_TEST` stability `internal`
- `const PUBLIC_MODULE_087_TEST` signature `const PUBLIC_MODULE_087_TEST: string = "src/vitte/stdlib/math/number_theory.vitl"` example `public_module_coverage.PUBLIC_MODULE_087_TEST` stability `internal`
- `const PUBLIC_MODULE_088_TEST` signature `const PUBLIC_MODULE_088_TEST: string = "src/vitte/stdlib/math/powers.vitl"` example `public_module_coverage.PUBLIC_MODULE_088_TEST` stability `internal`
- `const PUBLIC_MODULE_089_TEST` signature `const PUBLIC_MODULE_089_TEST: string = "src/vitte/stdlib/math/probability.vitl"` example `public_module_coverage.PUBLIC_MODULE_089_TEST` stability `internal`
- `const PUBLIC_MODULE_090_TEST` signature `const PUBLIC_MODULE_090_TEST: string = "src/vitte/stdlib/math/roots.vitl"` example `public_module_coverage.PUBLIC_MODULE_090_TEST` stability `internal`
- `const PUBLIC_MODULE_091_TEST` signature `const PUBLIC_MODULE_091_TEST: string = "src/vitte/stdlib/math/sequences.vitl"` example `public_module_coverage.PUBLIC_MODULE_091_TEST` stability `internal`
- `const PUBLIC_MODULE_092_TEST` signature `const PUBLIC_MODULE_092_TEST: string = "src/vitte/stdlib/math/sort.vitl"` example `public_module_coverage.PUBLIC_MODULE_092_TEST` stability `internal`
- `const PUBLIC_MODULE_093_TEST` signature `const PUBLIC_MODULE_093_TEST: string = "src/vitte/stdlib/math/statistics.vitl"` example `public_module_coverage.PUBLIC_MODULE_093_TEST` stability `internal`
- `const PUBLIC_MODULE_094_TEST` signature `const PUBLIC_MODULE_094_TEST: string = "src/vitte/stdlib/math/topology.vitl"` example `public_module_coverage.PUBLIC_MODULE_094_TEST` stability `internal`
- `const PUBLIC_MODULE_095_TEST` signature `const PUBLIC_MODULE_095_TEST: string = "src/vitte/stdlib/math/trigonometry.vitl"` example `public_module_coverage.PUBLIC_MODULE_095_TEST` stability `internal`
- `const PUBLIC_MODULE_096_TEST` signature `const PUBLIC_MODULE_096_TEST: string = "src/vitte/stdlib/math/vector.vitl"` example `public_module_coverage.PUBLIC_MODULE_096_TEST` stability `internal`
- `const PUBLIC_MODULE_097_TEST` signature `const PUBLIC_MODULE_097_TEST: string = "src/vitte/stdlib/network/http.vitl"` example `public_module_coverage.PUBLIC_MODULE_097_TEST` stability `internal`
- `const PUBLIC_MODULE_098_TEST` signature `const PUBLIC_MODULE_098_TEST: string = "src/vitte/stdlib/network/socket.vitl"` example `public_module_coverage.PUBLIC_MODULE_098_TEST` stability `internal`
- `const PUBLIC_MODULE_099_TEST` signature `const PUBLIC_MODULE_099_TEST: string = "src/vitte/stdlib/network/udp.vitl"` example `public_module_coverage.PUBLIC_MODULE_099_TEST` stability `internal`
- `const PUBLIC_MODULE_100_TEST` signature `const PUBLIC_MODULE_100_TEST: string = "src/vitte/stdlib/path/globbing.vitl"` example `public_module_coverage.PUBLIC_MODULE_100_TEST` stability `internal`
- `const PUBLIC_MODULE_101_TEST` signature `const PUBLIC_MODULE_101_TEST: string = "src/vitte/stdlib/path/manipulation.vitl"` example `public_module_coverage.PUBLIC_MODULE_101_TEST` stability `internal`
- `const PUBLIC_MODULE_102_TEST` signature `const PUBLIC_MODULE_102_TEST: string = "src/vitte/stdlib/path/special.vitl"` example `public_module_coverage.PUBLIC_MODULE_102_TEST` stability `internal`
- `const PUBLIC_MODULE_103_TEST` signature `const PUBLIC_MODULE_103_TEST: string = "src/vitte/stdlib/path/walker.vitl"` example `public_module_coverage.PUBLIC_MODULE_103_TEST` stability `internal`
- `const PUBLIC_MODULE_104_TEST` signature `const PUBLIC_MODULE_104_TEST: string = "src/vitte/stdlib/platform/abi.vitl"` example `public_module_coverage.PUBLIC_MODULE_104_TEST` stability `internal`
- `const PUBLIC_MODULE_105_TEST` signature `const PUBLIC_MODULE_105_TEST: string = "src/vitte/stdlib/platform/embedded.vitl"` example `public_module_coverage.PUBLIC_MODULE_105_TEST` stability `internal`
- `const PUBLIC_MODULE_106_TEST` signature `const PUBLIC_MODULE_106_TEST: string = "src/vitte/stdlib/platform/posix.vitl"` example `public_module_coverage.PUBLIC_MODULE_106_TEST` stability `internal`
- `const PUBLIC_MODULE_107_TEST` signature `const PUBLIC_MODULE_107_TEST: string = "src/vitte/stdlib/platform/wasm.vitl"` example `public_module_coverage.PUBLIC_MODULE_107_TEST` stability `internal`
- `const PUBLIC_MODULE_108_TEST` signature `const PUBLIC_MODULE_108_TEST: string = "src/vitte/stdlib/platform/windows.vitl"` example `public_module_coverage.PUBLIC_MODULE_108_TEST` stability `internal`
- `const PUBLIC_MODULE_109_TEST` signature `const PUBLIC_MODULE_109_TEST: string = "src/vitte/stdlib/profiling/profiler.vitl"` example `public_module_coverage.PUBLIC_MODULE_109_TEST` stability `internal`
- `const PUBLIC_MODULE_110_TEST` signature `const PUBLIC_MODULE_110_TEST: string = "src/vitte/stdlib/std/atomic.vitl"` example `public_module_coverage.PUBLIC_MODULE_110_TEST` stability `internal`
- `const PUBLIC_MODULE_111_TEST` signature `const PUBLIC_MODULE_111_TEST: string = "src/vitte/stdlib/std/base64.vitl"` example `public_module_coverage.PUBLIC_MODULE_111_TEST` stability `internal`
- `const PUBLIC_MODULE_112_TEST` signature `const PUBLIC_MODULE_112_TEST: string = "src/vitte/stdlib/std/bench.vitl"` example `public_module_coverage.PUBLIC_MODULE_112_TEST` stability `internal`
- `const PUBLIC_MODULE_113_TEST` signature `const PUBLIC_MODULE_113_TEST: string = "src/vitte/stdlib/std/calendar.vitl"` example `public_module_coverage.PUBLIC_MODULE_113_TEST` stability `internal`
- `const PUBLIC_MODULE_114_TEST` signature `const PUBLIC_MODULE_114_TEST: string = "src/vitte/stdlib/std/cli.vitl"` example `public_module_coverage.PUBLIC_MODULE_114_TEST` stability `internal`
- `const PUBLIC_MODULE_115_TEST` signature `const PUBLIC_MODULE_115_TEST: string = "src/vitte/stdlib/std/csv.vitl"` example `public_module_coverage.PUBLIC_MODULE_115_TEST` stability `internal`
- `const PUBLIC_MODULE_116_TEST` signature `const PUBLIC_MODULE_116_TEST: string = "src/vitte/stdlib/std/diff.vitl"` example `public_module_coverage.PUBLIC_MODULE_116_TEST` stability `internal`
- `const PUBLIC_MODULE_117_TEST` signature `const PUBLIC_MODULE_117_TEST: string = "src/vitte/stdlib/std/env.vitl"` example `public_module_coverage.PUBLIC_MODULE_117_TEST` stability `internal`
- `const PUBLIC_MODULE_118_TEST` signature `const PUBLIC_MODULE_118_TEST: string = "src/vitte/stdlib/std/error.vitl"` example `public_module_coverage.PUBLIC_MODULE_118_TEST` stability `internal`
- `const PUBLIC_MODULE_119_TEST` signature `const PUBLIC_MODULE_119_TEST: string = "src/vitte/stdlib/std/event.vitl"` example `public_module_coverage.PUBLIC_MODULE_119_TEST` stability `internal`
- `const PUBLIC_MODULE_120_TEST` signature `const PUBLIC_MODULE_120_TEST: string = "src/vitte/stdlib/std/format.vitl"` example `public_module_coverage.PUBLIC_MODULE_120_TEST` stability `internal`
- `const PUBLIC_MODULE_121_TEST` signature `const PUBLIC_MODULE_121_TEST: string = "src/vitte/stdlib/std/fs.vitl"` example `public_module_coverage.PUBLIC_MODULE_121_TEST` stability `internal`
- `const PUBLIC_MODULE_122_TEST` signature `const PUBLIC_MODULE_122_TEST: string = "src/vitte/stdlib/std/glob.vitl"` example `public_module_coverage.PUBLIC_MODULE_122_TEST` stability `internal`
- `const PUBLIC_MODULE_123_TEST` signature `const PUBLIC_MODULE_123_TEST: string = "src/vitte/stdlib/std/hash.vitl"` example `public_module_coverage.PUBLIC_MODULE_123_TEST` stability `internal`
- `const PUBLIC_MODULE_124_TEST` signature `const PUBLIC_MODULE_124_TEST: string = "src/vitte/stdlib/std/http.vitl"` example `public_module_coverage.PUBLIC_MODULE_124_TEST` stability `internal`
- `const PUBLIC_MODULE_125_TEST` signature `const PUBLIC_MODULE_125_TEST: string = "src/vitte/stdlib/std/io.vitl"` example `public_module_coverage.PUBLIC_MODULE_125_TEST` stability `internal`
- `const PUBLIC_MODULE_126_TEST` signature `const PUBLIC_MODULE_126_TEST: string = "src/vitte/stdlib/std/kernel.vitl"` example `public_module_coverage.PUBLIC_MODULE_126_TEST` stability `internal`
- `const PUBLIC_MODULE_127_TEST` signature `const PUBLIC_MODULE_127_TEST: string = "src/vitte/stdlib/std/locale.vitl"` example `public_module_coverage.PUBLIC_MODULE_127_TEST` stability `internal`
- `const PUBLIC_MODULE_128_TEST` signature `const PUBLIC_MODULE_128_TEST: string = "src/vitte/stdlib/std/log.vitl"` example `public_module_coverage.PUBLIC_MODULE_128_TEST` stability `internal`
- `const PUBLIC_MODULE_129_TEST` signature `const PUBLIC_MODULE_129_TEST: string = "src/vitte/stdlib/std/metrics.vitl"` example `public_module_coverage.PUBLIC_MODULE_129_TEST` stability `internal`
- `const PUBLIC_MODULE_130_TEST` signature `const PUBLIC_MODULE_130_TEST: string = "src/vitte/stdlib/std/mime.vitl"` example `public_module_coverage.PUBLIC_MODULE_130_TEST` stability `internal`
- `const PUBLIC_MODULE_131_TEST` signature `const PUBLIC_MODULE_131_TEST: string = "src/vitte/stdlib/std/net.vitl"` example `public_module_coverage.PUBLIC_MODULE_131_TEST` stability `internal`
- `const PUBLIC_MODULE_132_TEST` signature `const PUBLIC_MODULE_132_TEST: string = "src/vitte/stdlib/std/parse.vitl"` example `public_module_coverage.PUBLIC_MODULE_132_TEST` stability `internal`
- `const PUBLIC_MODULE_133_TEST` signature `const PUBLIC_MODULE_133_TEST: string = "src/vitte/stdlib/std/path.vitl"` example `public_module_coverage.PUBLIC_MODULE_133_TEST` stability `internal`
- `const PUBLIC_MODULE_134_TEST` signature `const PUBLIC_MODULE_134_TEST: string = "src/vitte/stdlib/std/percent_encoding.vitl"` example `public_module_coverage.PUBLIC_MODULE_134_TEST` stability `internal`
- `const PUBLIC_MODULE_135_TEST` signature `const PUBLIC_MODULE_135_TEST: string = "src/vitte/stdlib/std/process.vitl"` example `public_module_coverage.PUBLIC_MODULE_135_TEST` stability `internal`
- `const PUBLIC_MODULE_136_TEST` signature `const PUBLIC_MODULE_136_TEST: string = "src/vitte/stdlib/std/random.vitl"` example `public_module_coverage.PUBLIC_MODULE_136_TEST` stability `internal`
- `const PUBLIC_MODULE_137_TEST` signature `const PUBLIC_MODULE_137_TEST: string = "src/vitte/stdlib/std/serialization.vitl"` example `public_module_coverage.PUBLIC_MODULE_137_TEST` stability `internal`
- `const PUBLIC_MODULE_138_TEST` signature `const PUBLIC_MODULE_138_TEST: string = "src/vitte/stdlib/std/semver.vitl"` example `public_module_coverage.PUBLIC_MODULE_138_TEST` stability `internal`
- `const PUBLIC_MODULE_139_TEST` signature `const PUBLIC_MODULE_139_TEST: string = "src/vitte/stdlib/std/signal.vitl"` example `public_module_coverage.PUBLIC_MODULE_139_TEST` stability `internal`
- `const PUBLIC_MODULE_140_TEST` signature `const PUBLIC_MODULE_140_TEST: string = "src/vitte/stdlib/std/sync.vitl"` example `public_module_coverage.PUBLIC_MODULE_140_TEST` stability `internal`
- `const PUBLIC_MODULE_141_TEST` signature `const PUBLIC_MODULE_141_TEST: string = "src/vitte/stdlib/std/tempfile.vitl"` example `public_module_coverage.PUBLIC_MODULE_141_TEST` stability `internal`
- `const PUBLIC_MODULE_142_TEST` signature `const PUBLIC_MODULE_142_TEST: string = "src/vitte/stdlib/std/testing.vitl"` example `public_module_coverage.PUBLIC_MODULE_142_TEST` stability `internal`
- `const PUBLIC_MODULE_143_TEST` signature `const PUBLIC_MODULE_143_TEST: string = "src/vitte/stdlib/std/thread.vitl"` example `public_module_coverage.PUBLIC_MODULE_143_TEST` stability `internal`
- `const PUBLIC_MODULE_144_TEST` signature `const PUBLIC_MODULE_144_TEST: string = "src/vitte/stdlib/std/time.vitl"` example `public_module_coverage.PUBLIC_MODULE_144_TEST` stability `internal`
- `const PUBLIC_MODULE_145_TEST` signature `const PUBLIC_MODULE_145_TEST: string = "src/vitte/stdlib/std/terminal.vitl"` example `public_module_coverage.PUBLIC_MODULE_145_TEST` stability `internal`
- `const PUBLIC_MODULE_146_TEST` signature `const PUBLIC_MODULE_146_TEST: string = "src/vitte/stdlib/std/units.vitl"` example `public_module_coverage.PUBLIC_MODULE_146_TEST` stability `internal`
- `const PUBLIC_MODULE_147_TEST` signature `const PUBLIC_MODULE_147_TEST: string = "src/vitte/stdlib/std/uri.vitl"` example `public_module_coverage.PUBLIC_MODULE_147_TEST` stability `internal`
- `const PUBLIC_MODULE_148_TEST` signature `const PUBLIC_MODULE_148_TEST: string = "src/vitte/stdlib/std/url.vitl"` example `public_module_coverage.PUBLIC_MODULE_148_TEST` stability `internal`
- `const PUBLIC_MODULE_149_TEST` signature `const PUBLIC_MODULE_149_TEST: string = "src/vitte/stdlib/std/uuid.vitl"` example `public_module_coverage.PUBLIC_MODULE_149_TEST` stability `internal`
- `const PUBLIC_MODULE_EXTRA_158_TEST` signature `const PUBLIC_MODULE_EXTRA_158_TEST: string = "src/vitte/stdlib/alloc/btree.vitl"` example `public_module_coverage.PUBLIC_MODULE_EXTRA_158_TEST` stability `internal`
- `const PUBLIC_MODULE_EXTRA_159_TEST` signature `const PUBLIC_MODULE_EXTRA_159_TEST: string = "src/vitte/stdlib/alloc/deque.vitl"` example `public_module_coverage.PUBLIC_MODULE_EXTRA_159_TEST` stability `internal`
- `const PUBLIC_MODULE_EXTRA_160_TEST` signature `const PUBLIC_MODULE_EXTRA_160_TEST: string = "src/vitte/stdlib/alloc/smallvec.vitl"` example `public_module_coverage.PUBLIC_MODULE_EXTRA_160_TEST` stability `internal`
- `proc public_module_tests_present` signature `proc public_module_tests_present() -> bool {` example `public_module_coverage.public_module_tests_present(...)` stability `internal`

## `src/vitte/stdlib/tests/range_unicode_std_contracts.vit`

Stability: `internal`

- `proc stdlib_range_unicode_std_contracts_smoke` signature `proc stdlib_range_unicode_std_contracts_smoke() -> bool {` example `range_unicode_std_contracts.stdlib_range_unicode_std_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/serialization_platform_contracts.vit`

Stability: `internal`

- `proc stdlib_serialization_platform_contracts_smoke` signature `proc stdlib_serialization_platform_contracts_smoke() -> bool {` example `serialization_platform_contracts.stdlib_serialization_platform_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/std_extra_libraries_contracts.vit`

Stability: `internal`

- `proc stdlib_extra_libraries_contracts_smoke` signature `proc stdlib_extra_libraries_contracts_smoke() -> bool {` example `std_extra_libraries_contracts.stdlib_extra_libraries_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/std_more_libraries_contracts.vit`

Stability: `internal`

- `proc stdlib_more_libraries_contracts_smoke` signature `proc stdlib_more_libraries_contracts_smoke() -> bool {` example `std_more_libraries_contracts.stdlib_more_libraries_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tests/std_runtime_contracts.vit`

Stability: `internal`

- `proc stdlib_runtime_contracts_smoke` signature `proc stdlib_runtime_contracts_smoke() -> bool {` example `std_runtime_contracts.stdlib_runtime_contracts_smoke(...)` stability `internal`

## `src/vitte/stdlib/tools/unicode_tables.vitl`

Stability: `internal`

- `form UnicodeTableGeneration` signature `form UnicodeTableGeneration {` example `unicode_tables.UnicodeTableGeneration` stability `internal`
- `proc unicode_table_generation` signature `proc unicode_table_generation(version: string, source: string, output: string) -> UnicodeTableGeneration {` example `unicode_tables.unicode_table_generation(...)` stability `internal`
- `proc generate_unicode_tables` signature `proc generate_unicode_tables(job: UnicodeTableGeneration) -> Result<(), string> {` example `unicode_tables.generate_unicode_tables(...)` stability `internal`
- `proc verify_unicode_tables` signature `proc verify_unicode_tables(version: string) -> bool {` example `unicode_tables.verify_unicode_tables(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/vec_bench.vit`

Stability: `internal`

- `proc bench_vec_push_pop` signature `proc bench_vec_push_pop(iterations: usize) -> usize {` example `vec_bench.bench_vec_push_pop(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/string_bench.vit`

Stability: `internal`

- `proc bench_string_push_concat` signature `proc bench_string_push_concat(iterations: usize) -> usize {` example `string_bench.bench_string_push_concat(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/hashmap_bench.vit`

Stability: `internal`

- `proc bench_hashmap_insert_get` signature `proc bench_hashmap_insert_get(iterations: usize) -> usize {` example `hashmap_bench.bench_hashmap_insert_get(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/utf8_bench.vit`

Stability: `internal`

- `proc bench_utf8_validate_decode` signature `proc bench_utf8_validate_decode(iterations: usize) -> usize {` example `utf8_bench.bench_utf8_validate_decode(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/path_bench.vit`

Stability: `internal`

- `proc bench_path_normalize_join` signature `proc bench_path_normalize_join(iterations: usize) -> usize {` example `path_bench.bench_path_normalize_join(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/format_bench.vit`

Stability: `internal`

- `proc bench_format_ints` signature `proc bench_format_ints(iterations: usize) -> usize {` example `format_bench.bench_format_ints(...)` stability `internal`

## `src/vitte/stdlib/benchmarks/modules/parse_bench.vit`

Stability: `internal`

- `proc bench_parse_numbers` signature `proc bench_parse_numbers(iterations: usize) -> usize {` example `parse_bench.bench_parse_numbers(...)` stability `internal`
